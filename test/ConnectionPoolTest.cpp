#include <iostream>

#include "../ConnectionPool.hpp"

using namespace std;
using namespace pool;

int main()
{
    Connection* con=new Connection();
    con->connect("192.168.28.128","root","123456","testdb");
    con->update("insert into user(username,passwd) values('ttt','1134')");
    delete con;
    return 0;
}