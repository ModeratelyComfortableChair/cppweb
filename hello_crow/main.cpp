#include "crow_all.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <unordered_set>
#include <mutex>
#include <string>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::kvp;
using mongocxx::cursor;

using namespace std;
using namespace crow;
using namespace crow::mustache;

void getView(response &res, const string &filename, context &x){
  res.set_header("Content-Type", "text/html");
  auto text = load("../public/" + filename + ".html").render(x);
  res.write(text);
  res.end();
}

const uint16_t DEFAULT_PORT = 8080;
const string DEFAULT_MONGODB_URI_GUEST = "mongodb+srv://guest:DxLB2kd6cnB1@cpp-api-tutorial-l3zax.mongodb.net";

void sendFile(response &res, string filename, string contentType){
  ifstream in("../public/" + filename, ifstream::in);
  if(in){
    ostringstream contents;
    contents << in.rdbuf();
    in.close();
    res.set_header("Content-Type", contentType);
    res.write(contents.str());
  } else {
    res.code = 404;
    res.write("Not found");
  }
  res.end();
}

void sendHtml(response &res, string filename){
  sendFile(res, filename + ".html", "text/html");
}

void sendImage(response &res, string filename){
  sendFile(res, "images/"+ filename, "image/jpeg");
}

void sendStyle(response &res, string filename){
  sendFile(res, "styles/" + filename, "text/css");
}

void sendScript(response &res, string filename){
  sendFile(res, "scripts/"+ filename, "text/javascript");
}

void notFound(response &res, const string &message){
  res.code = 404;
  res.write(message +": Not Found");
  res.end();
}

int main(int argc, char* argv[]){

  std::mutex mtx;
  std::unordered_set<crow::websocket::connection *> users;

  crow::SimpleApp app;

  //need this for mustache to work
  set_base(".");

  mongocxx::instance inst{};
  string mongoConnect;
  mongoConnect = std::string(getenv("MONGODB_URI"));

  /*try{
    mongoConnect = std::string(getenv("MONGODB_URI"));
  } catch (const std::exception& e){
    mongoConnect =  DEFAULT_MONGODB_URI_GUEST;
  }*/
  mongocxx::client conn{mongocxx::uri{mongoConnect}};
  auto collection = conn["cpp-api-tutorial"]["contacts"];

  CROW_ROUTE(app, "/ws")
    .websocket()
    .onopen([&](crow::websocket::connection & conn){
      std::lock_guard<std::mutex>_(mtx);
      users.insert(&conn);
    })
    .onclose([&](crow::websocket::connection & conn, const string &reason){
      std::lock_guard<std::mutex>_(mtx);
      users.erase(&conn);
    })
    .onmessage([&](crow::websocket::connection &/*conn*/, const string &data, bool is_binary){
      std::lock_guard<std::mutex>_(mtx);
      for(auto user : users){
        if(is_binary){
          user->send_binary(data);
        } else {
          user->send_text(data);
        }
      }
    });

  CROW_ROUTE(app, "/chat")
  ([](const request &req, response &res){
    sendHtml(res, "chat");
  });

  CROW_ROUTE(app, "/images/<string>")
  ([](const request &req, response &res, string filename){
    sendImage(res, filename);
  });

  CROW_ROUTE(app, "/styles/<string>")
  ([](const request &req, response &res, string filename){
    sendStyle(res, filename);
  });

  CROW_ROUTE(app, "/scripts/<string>")
  ([](const request &req, response &res, string filename){
    sendScript(res, filename);
  });

  CROW_ROUTE(app, "/add/<int>/<int>")
  ([](const request &req, response &res, int a, int b){
    res.set_header("Content-Type", "text/plain");
    ostringstream os;
    os << "Integer: " << a << " + " << b << " = " << a + b << "\n";
    res.write(os.str());
    res.end();
  });

  CROW_ROUTE(app, "/add/<double>/<double>")
  ([](const request &req, response &res, double a, double b){
    res.set_header("Content-Type", "text/plain");
    ostringstream os;
    os << "Double: " << a << " + " << b << " = " << a + b << "\n";
    res.write(os.str());
    res.end();
  });

  //keep string lasts
  CROW_ROUTE(app, "/add/<string>/<string>")
  ([](const request &req, response &res, string a, string b){
    res.set_header("Content-Type", "text/plain");
    ostringstream os;
    os << "String: " << a << " + " << b << " = " << a + b << "\n";
    res.write(os.str());
    res.end();
  });

  CROW_ROUTE(app, "/query")
  ([](const request &req, response &res){
    auto firstname = req.url_params.get("firstname");
    auto lastname = req.url_params.get("lastname");
    ostringstream os;
    os << "Hello " << (firstname? firstname: "") <<
    " " << (lastname? lastname: "") << endl;
    res.set_header("Content-Type", "text/plain");
    res.write(os.str());
    res.end();
  });

  CROW_ROUTE(app, "/rest_test").methods(HTTPMethod::Post, HTTPMethod::Get, HTTPMethod::Put)
  ([](const request &req, response &res){
    string method = method_name(req.method);
    res.set_header("Content-Type", "text/plain");
    res.write(method + " rest_test");
    res.end();
  });

  CROW_ROUTE(app, "/")
  ([](const request &req, response &res){
    sendHtml(res, "index");
  });

  CROW_ROUTE(app, "/about")
  ([](const request &req, response &res){
    sendHtml(res, "about");
  });

  CROW_ROUTE(app, "/contact/<string>")
  ([&collection](const request &req, response &res, string email){
    auto doc = collection.find_one(make_document(kvp("email", email)));
    crow::json::wvalue dto;
    dto["contact"] = json::load(bsoncxx::to_json(doc.value().view()));
    getView(res, "contact", dto);
  });

  CROW_ROUTE(app, "/contact/<string>/<string>")
  ([&collection](const request &req, response &res, string firstname, string lastname){
    auto doc = collection.find_one(
      make_document(kvp("firstName", firstname), kvp("lastName", lastname)));
    if(!doc){
      return notFound(res, "Contact");
    }
    
    crow::json::wvalue dto;
    dto["contact"] = json::load(bsoncxx::to_json(doc.value().view()));
    getView(res, "contact", dto);
  });

  CROW_ROUTE(app, "/contacts")
  ([&collection](const request &req, response &res){
    mongocxx::options::find opts;
    opts.limit(10);
    auto docs = collection.find({}, opts);
    crow::json::wvalue dto;
    vector<crow::json::rvalue> contacts;
    contacts.reserve(10);

    for(auto doc : docs){
      contacts.push_back(json::load(bsoncxx::to_json(doc)));
    }
    dto["contacts"] = contacts;
    getView(res, "contacts", dto);
  });

  CROW_ROUTE(app, "/api/contacts")
  ([&collection](const request &req){
    mongocxx::options::find opts;
    auto skip = req.url_params.get("skip");
    auto limit = req.url_params.get("limit");
    opts.skip((skip ? std::atoi(skip) : 0));
    opts.limit((limit ? std::atoi(limit) : 0));
    auto docs = collection.find({}, opts);
    vector<crow::json::rvalue> contacts;
    contacts.reserve(10);

    for(auto doc : docs){
      contacts.push_back(json::load(bsoncxx::to_json(doc)));
    }

    crow::json::wvalue dto;
    dto["contacts"] = contacts;
    return crow::response{dto};
  });

  char* port = getenv("PORT");
  uint16_t iPort = static_cast<uint16_t>(port != NULL? stoi(port) : DEFAULT_PORT);
  cout << "PORT = " << iPort << "\n";
  app.port(iPort).multithreaded().run();
}