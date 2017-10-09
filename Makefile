all: yash.c input.c process.c
	gcc -g -Wall -o yash yash.c input.c 
	gcc -g -Wall -o yashd yashd.c input.c process.c logger.c -pthread -lrt

yashd.o : yashd.c input.h process.h logger.h
	cc -c yashd.c -pthread -lrt

yash.o : yash.c input.h process.h logger.h
	cc -c yash.c
	


clean:
	$(RM) yash
	$(RM) yashd
