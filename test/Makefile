.PHONY: all
all:connectionpooltest semaphoretest threadpooltest

connectionpooltest: ConnectionPoolTest.cpp ../ConnectionPool.hpp
	g++ -o $@ $^ -lmysqlclient

semaphoretest: SemaphoreTest.cpp ../Semaphore.hpp
	g++ -o $@ $^ -lpthread

threadpooltest: ThreadPoolTest.cpp ../ThreadPool.hpp
	g++ -o $@ $^ -lpthread

.PHONY:clean
clean:
	rm -f connectionpooltest semaphoretest threadpooltest