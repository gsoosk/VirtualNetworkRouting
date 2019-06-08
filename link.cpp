#include "link.hpp"
using namespace std;

Link::Link(int port) {
  // Creating socket file descriptor
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // memset(&address, 0, sizeof(address));

  // Filling server information
  address.sin_family = AF_INET; // IPv4
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // Bind the socket with the server address
  if (bind(sockfd, (const struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
}

void Link::send_routing_table(map<int, routing_table_info> routing_table,
                              string ip, int port) {
  string r_string = serialize_routing_table(routing_table);
  iphdr header;
  header.protocol = IPPROTO_ROUTING_TABLE;
  send_data(header, r_string, ip, port);
}

void Link::send_nodes_info(
    std::map<std::string, struct node_physical_info> nodes_info, std::string ip,
    int port) {
  string n_string = serialize_nodes_info(nodes_info);
  iphdr header;
  header.protocol = IPPROTO_NODES_INFO;
  send_data(header, n_string, ip, port);
}

string
Link::serialize_routing_table(map<int, routing_table_info> routing_table) {
  string data = "";
  for (auto it = routing_table.begin();
       !routing_table.empty() && it != routing_table.end(); it++) {
    data += to_string(it->first) + " " + to_string(it->second.best_route_port) +
            " " + to_string(it->second.cost) + "\n";
  }
  return data;
}

string Link::serialize_nodes_info(map<string, node_physical_info> nodes_info) {
  string data = "";
  for (auto it = nodes_info.begin();
       !nodes_info.empty() && it != nodes_info.end(); it++) {
    data += it->first + " " + it->second.phys_ip + " " +
            to_string(it->second.port) + "\n";
  }
  return data;
}

vector<string> Link::tokenize(const string &cnt, char delimiter) {
  vector<string> res;
  istringstream is(cnt);
  string part;
  while (getline(is, part, delimiter))
    res.push_back(part);
  return res;
}

map<int, routing_table_info> Link::deserialize_routing_table(string data) {
  map<int, routing_table_info> m;
  vector<string> rows;
  rows = tokenize(data, '\n');
  for (unsigned int i = 0; i < rows.size(); i++) {
    vector<string> cols = tokenize(rows[i], ' ');
    routing_table_info r;
    r.best_route_port = stoi(cols[1]);
    r.cost = stoi(cols[2]);
    m[stoi(cols[0])] = r;
  }
  return m;
}

map<string, node_physical_info> Link::deserialize_nodes_info(string data) {
  map<string, node_physical_info> m;
  vector<string> rows;
  rows = tokenize(data, '\n');
  for (unsigned int i = 0; i < rows.size(); i++) {
    vector<string> cols = tokenize(rows[i], ' ');
    node_physical_info n;
    n.phys_ip = cols[1];
    n.port = stoi(cols[2]);
    m[cols[0]] = n;
  }
  return m;
}

int Link::send_data(iphdr header, string data, string ip, int port) {
  struct sockaddr_in client_addr;

  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = inet_addr(ip.c_str());
  client_addr.sin_port = htons(port);

  char *toSend;
  toSend = (char *) malloc(sizeof(iphdr) + data.size() + 1);
  memcpy(toSend, (char *)&header, sizeof(iphdr));
  memcpy(toSend + sizeof(iphdr), data.c_str(), data.size() + 1);
  // TODO: Send data more than buffer
  
  int size = sendto(sockfd, toSend, sizeof(iphdr) + data.size() + 1, 0,
                    (const struct sockaddr *)&client_addr, sizeof(client_addr));
  return size;
}

void Link::recv_data() {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(client_addr)); 

  char buffer[1400];
  int size;
  unsigned int len;
  size = recvfrom(sockfd, (char *)buffer, 1400, 0,
                  (struct sockaddr *)&client_addr, &len);
  cerr << size << endl;
  // TODO: recive data more than buffer

  iphdr rec_header;
  char *rec_data = (char *)malloc(size + 1 - sizeof(iphdr));

  memcpy(&rec_header, buffer, sizeof(iphdr));
  memcpy(rec_data, buffer + sizeof(iphdr), size - sizeof(iphdr));
  rec_data[size + 1 - sizeof(iphdr)] = '/0';

  string rec_data_str = rec_data;

  cout << "header : " << (int) rec_header.protocol << endl << rec_data_str << endl;

  for (unsigned int i = 0; i < handlers.size(); i++) {
    if (handlers[i].protocol_num == rec_header.protocol) {
      handlers[i].handler(rec_data_str, rec_header);
      return;
    }
  }
}

void Link::register_handler(protocol_handler handler) {
  handlers.push_back(handler);
}