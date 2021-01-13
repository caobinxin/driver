OBJ =test
HEADERS =
CC = arm-none-linux-gnueabi-gcc
CXX = arm-none-linux-gnueabi-g++
CFLAGS = -g
LDFLAGS = -lpthread
all: ${OBJ}
${OBJ}: %.o: %.c

clean:
	rm -f *.o 
	rm -f ${OBJ}

%.o:    %.cpp ${HEADERS}
	${CXX} ${CFLAGS} ${LDFLAGS} -c $< -o $@

%.o:    %.c ${HEADERS}
	${CC} ${CFLAGS} ${LDFLAGS} -c $< -o $@
