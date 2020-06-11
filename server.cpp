//c
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

//c++
#include <iostream>
#include <vector>
#include <string>
#include <ctime>

//linux and internet
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

//external libs
#include "json.hpp"

//header files
#include "helpers.hpp"
#include "client.hpp"
#include "database.hpp"
#include "rate_limit.hpp"

#define null NULL //mandatory 
#define PORT 9998
#define REQUEST_LIMIT 10
#define TIME_LIMIT 900 //60 seconds * 15 minutes 

using namespace std;
using namespace nlohmann;

//global so I can acces it everywhere without an extra argument
Database database;
RateLimiter limiter;
string dummy;

string get_date_string()
{
	time_t rawtime;
	struct tm * timeinfo;
  	char buffer[80];

  	time (&rawtime);
  	timeinfo = localtime(&rawtime);

  	strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
  	std::string str(buffer);
	return str;
}

string compute_response(string status_code, string explanation, bool extra, string extra_stuff)
{
	//version, status code, explanation
	string response;
	response.append("HTTP/1.1");
	response.append(status_code);
	response.append(" ");
	response.append(explanation);
	response.append(" ");
	response.append("\r\n");

	//connection
	response.append("Connection: Keep-alive\r\n");

	//content-type
	response.append("Content-type: application/json\r\n");

	//date
	response.append("Date: ");
	
	string str = get_date_string();

	response.append(str);
	response.append("\r\n");

	if(extra)
	{
		response.append(extra_stuff);
		response.append("\r\n");
	}

	//i dont really know what else to add :(
	return response;
}

string no_json_data()
{
	json data;
	data["error"] = "No JSON data.";
	string response = compute_response("400", "Bad request", false, dummy);
	response.append("\n");
	response.append(data.dump());
	response.append("\r\n");
	return response;
}

string ok_response()
{
	string response = compute_response("200", "Ok", false, dummy);
	return response;
}

//generates a pseudo-random string
string generate_random_string()
{
	char letters[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
	'r','s','t','u','v','w','x',
	'y','z'};
	string random = "";
	for (int i = 0; i < 10; i++) 
		random	= random + letters[rand() % 26];

	return random;
}

bool cookie_check(char* buffer, int client)
{
	string cookie_string(buffer);
	int client_number = 0;
	for (size_t i = 0; i < database.clients.size(); i++)
	{
		if(database.clients[i].current_descriptor == client)
		{
			client_number = i;
			break;
		}
	}
	

	//check for cookie
	//if we can't find the curent cookie in the http request it means
	//it wasn't set
	if(cookie_string.find(database.clients[client_number].curent_cookie) == string::npos)
	{
		return false;
	}
	return true;
}

void command_doesnt_exist(int client)
{
	string response = compute_response("404", "Not found", false, dummy);
	//send
	int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
	DIE(check_value < 0, "send");
}

//check if the name exists in the database already or not
bool exists(string name)
{
	for (size_t i = 0; i < database.clients.size(); i++)
	{
		if(!database.clients[i].name.compare(name))
		{
			return true;
		}
	}
	return false;
}

void handle_register(char* buffer, int client)
{
	bool ok = true;
	char* position = strstr(buffer, "{");
	if(position == null)
	{
		ok = false; //if there is no json data
	}
	json user_data = json::parse(position);
	Client new_client;

	string name(user_data["name"]);
	string password(user_data["password"]);

	new_client.name = name;
	new_client.password = password;
	new_client.is_looged_in = false;
	new_client.current_descriptor = client;

	if(exists(name))
	{
		json data;
		data["error"] = "Username already exists.";
		string response = compute_response("400", "Bad request", true, data.dump());
		//send
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
	}
	else
	{
		database.clients.push_back(new_client);
	}

	if(ok)
	{
		string response = compute_response("200", "Ok", false, dummy);
		//send
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
	}
	else
	{
		string response = no_json_data();
		//send
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
	}
}

void handle_login(char* buffer, int client)
{
	char* position = strstr(buffer, "{");
	if(position == null)
	{
		string response = no_json_data();
		//send
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
		return;
	}
	json user_data = json::parse(position);
	
	//if the user isnt in the database
	if(!exists(user_data["name"]))
	{
		json data;
		data["error"] = "Username doesn't exist.";
		string response = compute_response("400", "Bad request", true, data.dump());
		//send
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
	}
	else //if the user is in the database
	{
		//generate a cookie for the client
		string cookie;
		cookie.append(generate_random_string());
		cookie.append(user_data["name"]);
		
		json data;
		data["cookie.sid"] = cookie;

		for (size_t i = 0; i < database.clients.size(); i++)
		{
			if(!database.clients[i].name.compare(user_data["name"]))
			{
				database.clients[i].is_looged_in = true;
				database.clients[i].curent_cookie = cookie;
				break;
			}
		}
		
		string response = ok_response();
		
		response.append("\r\n");
		response.append(data.dump());

		//send
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
	}

}

//return the index of the product if it exists
//else returns -1
int product_exists(string name)
{
	for (size_t i = 0; i < database.products.size(); i++)
	{
		if(!database.products[i].name.compare(name))
		{
			return i;
		}
	}
	return -1;
}

void handle_add_product(char* buffer, int client)
{
	if(cookie_check(buffer, client))
	{
		json data;
		data["error"] = "No cookie set";

		string response;
		response = compute_response("403", "Forbidden", true, data.dump());
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
		return;
	}

	char* position = strstr(buffer, "{");
	if(position == null)
	{
		string response = no_json_data();
		//send
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
		return;
	}
	json user_data = json::parse(position);

	//if the product exists just change the date
	int check_existance = product_exists(user_data["name"]);
	if(check_existance == -1)
	{
		//respond
		string response;
		response = ok_response();
		json data;
		data["warning"] = "Already exists, date updated";
		response.append("\r\n");
		response.append(data.dump());
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");

		//change the date
		database.products[check_existance].updatedDate = get_date_string();
		//id
		database.products[check_existance].id = database.products.size() + 1;
		//cathegory
		database.products[check_existance].category = user_data["category"];
		//name
		database.products[check_existance].name = user_data["name"];
		//price
		string price = user_data["price"];
		database.products[check_existance].price = atoi(price.c_str());
		return;
	}

	Product new_product;
	//id
	new_product.id = database.products.size() + 1;
	//cathegory
	new_product.category = user_data["category"];
	//name
	new_product.name = user_data["name"];
	//price
	string price = user_data["price"];
	new_product.price = atoi(price.c_str());
	//dates
	new_product.createdDate = get_date_string();
	new_product.updatedDate = get_date_string();

	string response;
	response = ok_response();
	int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
	DIE(check_value < 0, "send");
}

void handle_post(char* buffer, int client)
{
	char* aux = (char*)malloc(1500);
	memcpy(aux, buffer, strlen(buffer));

	char* location = strtok(aux, " ");
	location = strtok(NULL, " ");
	string finder(location);

	if(!finder.compare("api/register"))
	{
		handle_register(buffer, client);
	}
	else if(!finder.compare("api/login"))
	{
		handle_login(buffer, client);
	}
	else if(!finder.compare("api/add_product"))
	{
		handle_add_product(buffer, client);
	}
	else
	{
		command_doesnt_exist(client);
	}
	free(aux);
}

void handle_books(char* buffer, int client)
{
	if(cookie_check(buffer, client))
	{
		json data;
		data["error"] = "No cookie set";

		string response;
		response = compute_response("403", "Forbidden", true, data.dump());
		int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
		DIE(check_value < 0, "send");
		return;
	}
	//create a json array
	json array = json::array();
	for (size_t i = 0; i < database.products.size(); i++)
	{
		json product;
		product["id"] = database.products[i].id;
		product["name"] = database.products[i].name;
		product["price"] = database.products[i].price;
		product["category"] = database.products[i].category;
		product["created_date"] = database.products[i].createdDate;
		product["updated_date"] = database.products[i].updatedDate;
		array.push_back(product);
	}
	
	string response;
	response = ok_response();
	response.append("\r\n");
	response.append(array);
	int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
	DIE(check_value < 0, "send");
}


void handle_get(char* buffer, int client)
{
	char* aux = (char*)malloc(1500);
	memcpy(aux, buffer, strlen(buffer));

	char* location = strtok(aux, " ");
	location = strtok(NULL, " ");
	string finder(location);

	if(!finder.compare("api/products/books"))
	{
		handle_books(buffer, client);
	}
	else
	{
		command_doesnt_exist(client);
	}

	free(aux);
}

void handle_request(char* buffer, int client)
{
	char* aux = (char*)malloc(1500);
	memcpy(aux, buffer, strlen(buffer));

	//get the verb
	char* type = strtok(aux, " ");
	string finder(type);

	//handle request based on verb
	if(!finder.compare("POST"))
	{
		handle_post(buffer, client);
	}
	if(!finder.compare("GET"))
	{
		handle_get(buffer, client);
	}

	free(aux);
}

void dissconect(int client)
{
	for (size_t i = 0; i < database.clients.size(); i++)
	{
		if(database.clients[i].current_descriptor == client)
		{
			database.clients[i].is_looged_in = false;
			return;
		}
	}
}

void add_if_doesnt_exist(User new_user)
{
	int ok = 0;
	for (size_t i = 0; i < limiter.users.size(); i++)
	{
		if(limiter.users[i].ip == new_user.ip)
		{
			ok = i;
			break;
		}
	}
	//but if it exists we update the descriptor
	if(ok == 0)
	{
		limiter.users.push_back(new_user);
	}
	else
	{
		limiter.users[ok].descriptor = new_user.descriptor;
	}
}

//check if the user has any requests left
bool has_requests(int descriptor)
{
	for (size_t i = 0; i < limiter.users.size(); i++)
	{
		if(limiter.users[i].descriptor == descriptor)
		{
			if(limiter.users[i].request_limit > 0)
			{
				//you lose a request beacuase you just made one
				limiter.users[i].request_limit--;
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

void update_time()
{
	//time returns UNIX time
	long current_time = time(NULL);
	for (size_t i = 0; i < limiter.users.size(); i++)
	{
		if(current_time - limiter.users[i].time_here > TIME_LIMIT)
		{
			limiter.users[i].time_here = current_time;
			limiter.users[i].request_limit = REQUEST_LIMIT;
		}
	}	
}

void no_more_requests(int client)
{
	string response;
	response = compute_response("429", "Too Many Requests", false, dummy);
	int check_value = send(client, response.c_str(), sizeof(response.c_str()), 0);
	DIE(check_value < 0, "send");
}

int main()
{
	int server_socket;
	int i, check_value;
	struct sockaddr_in server_info, client_info;
	socklen_t lungime_sockaddr_client;

	fd_set desciptor_set;	// set used for select()
	fd_set temporary_set;		// temporary set
	int maximum_descriptor_value;

	//prepare the descriptor sets
	FD_ZERO(&desciptor_set);
	FD_ZERO(&temporary_set);

	//prepare the server for new connections
	//tcp server init
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(server_socket < 0, "socket");

	//preapre server info (port from the macro, ipv4)
	memset((char *) &server_info, 0, sizeof(server_info));
	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(PORT);
	server_info.sin_addr.s_addr = INADDR_ANY;

	//tcp server init
	check_value = bind(server_socket, (struct sockaddr *) &server_info, sizeof(struct sockaddr));
	DIE(check_value < 0, "bind");

	//tcp server init
	check_value = listen(server_socket, 1500);
	DIE(check_value < 0, "listen");

	// add the new file descriptor in the read set
	FD_SET(server_socket, &desciptor_set); //tcp server init
	// add the stdin file descriptor as well
	FD_SET(0, &desciptor_set);

	// we have in the set stdin stdout strerr server_socket clientsocket1 clicketsocket2....
	maximum_descriptor_value = server_socket;

	while (1) {
		temporary_set = desciptor_set; 

		update_time();
		
		check_value = select(maximum_descriptor_value + 1, &temporary_set, null, null, null);
		DIE(check_value < 0, "select");

		for (i = 0; i <= maximum_descriptor_value; i++) {
			if (FD_ISSET(i, &temporary_set)) {
				if(i == 0) // read comands to control the server (only exit is implemented)
				{
                    string standard_input;
                    cin >> standard_input;
                    if (standard_input.find("exit") != string::npos) {
                        cout << "Closing." << endl;
                        close(server_socket);
                        return 0;  
                    } 
				} else if (i == server_socket) {
					// new connection
					int new_socket_file_descriptor;
					lungime_sockaddr_client = sizeof(client_info);
					new_socket_file_descriptor = accept(server_socket, (struct sockaddr *) &client_info, &lungime_sockaddr_client);
					DIE(new_socket_file_descriptor < 0, "accept");

					cout << "Client on the socket " << new_socket_file_descriptor << " connected." << endl;
					// add the fd returned by accept to the set
					FD_SET(new_socket_file_descriptor, &desciptor_set);
					if (new_socket_file_descriptor > maximum_descriptor_value) {  //probably always true
						maximum_descriptor_value = new_socket_file_descriptor;
					}

					User new_user;
					new_user.ip = client_info.sin_addr.s_addr; //save the ip
					new_user.time_here = time(null);
					new_user.request_limit = REQUEST_LIMIT;
					new_user.descriptor = new_socket_file_descriptor;

					//or update the descriptor
					add_if_doesnt_exist(new_user);
				} 	
				else
				{
					// data recieved from a client
					char* buffer;
					buffer = (char*)malloc(1500);
					check_value = recv(i, buffer, 1500, 0);
					DIE(check_value < 0, "listen");

					if (check_value == 0) {
						cout << "Client on the socket " << i << " disconnected." << endl;
						dissconect(i);
						// remove from the set the client that just disconected
						FD_CLR(i, &desciptor_set);
					} else {
						if(has_requests(i))
						{
							handle_request(buffer, i);
						}
						else
						{
							no_more_requests(i);
						}
					}
					free(buffer);
				}
			}
		}
	}
}
