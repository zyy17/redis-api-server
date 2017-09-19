
### Deprecated

Future work will **continue** on [logcabin-redis-proxy](https://github.com/yunba/logcabin-redis-proxy)

## Overview

`redis-api-server` is the simple server framework that can ship your API with RESP protocol.

## How to run

```
$ mkdir build
$ cd build ; cmake ..
$ make
$ ./redis_api_server
```

```
$ redis-cli -p 6380  # server running on port 6380 by default
127.0.0.1:6380> set a b
OK
127.0.0.1:6380> get a
1) "foo"
127.0.0.1:6380> rpush test 1 2 3 4 5
OK
127.0.0.1:6380> LRANGE test 0 -1
1) "foo"
2) "bar"
3) "tom"
```

## More

- export handler function;

- handle more redis API;

- add more test cases;
