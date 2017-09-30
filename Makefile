all: yash.c input.c process.c
	gcc -g -Wall -o yash yash.c input.c
	gcc -g -Wall -o yashd yashd.c input.c process.c -pthread

yashd.o : yashd.c input.h process.h
	cc -c yashd.c -pthread

yash.o : yash.c input.h process.h
	cc -c yash.c
	


clean:
	$(RM) yash
	$(RM) yashd
