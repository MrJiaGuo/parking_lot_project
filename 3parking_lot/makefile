CC = arm-linux-gcc
SRC = ${wildcard *.c camera/*.c jpeg/*.c rfid/*.c database/*.c touch/*.c}
OBJS = ${patsubst %.c,%.o,${SRC}}
ELF = parkproject
CFLAGS = -c
RM = rm -rf
lib =-lpthread -ljpeg -lsqlite3


$(ELF):${OBJS}
	${CC} ${OBJS} -o $@ $(lib) -I./

%.o:%.c
	${CC} ${CFLAGS} $< -o $@ ${lib}

.PHONY:clean
clean:
	${RM} ${OBJS} ${ELF}



