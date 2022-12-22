CC 		:= gcc
CFLAGS 	:= -std=gnu99 -Wall -Wextra
LD 		:= gcc
LDFLAGS := -o main -g -lgcc
DEBUG	:= -DDEBUG

SRC=src
SRC_DIRS	:= $(SRC)

C_FILES 	:= $(wildcard $(addsuffix /*.c,$(SRC_DIRS)))

INCLUDES	:= $(addprefix -I/,$(SRC_DIRS))

ODIR=obj
C_OBJS 		:= $(addprefix $(ODIR)/, $(C_FILES:%.c=%.o))

BIN 		:= main


all: $(ODIR) $(BIN)

$(ODIR):
	mkdir -p $(addprefix $(ODIR)/, $(SRC))

$(BIN): $(C_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(DEBUG)

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^ $(INCLUDES) 

clean:
	rm -rf obj
	rm $(BIN)