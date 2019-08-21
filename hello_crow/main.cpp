#include "crow_all.h"
using namespace std;

const uint16_t DEFAULT_PORT = 18080;

int main(int argc, char* argv[]){

  crow::SimpleApp app;

  CROW_ROUTE(app, "/")
  ([](){
    return "<div><h1>Hello, Jerome!</h1></div>";
  });

  char* port = getenv("PORT");
  uint16_t iPort = static_cast<uint16_t>(port != NULL? stoi(port) : DEFAULT_PORT);
  cout << "PORT = " << iPort << "\n";
  app.port(iPort).multithreaded().run();
}