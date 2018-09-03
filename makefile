CFLAGS = -Wall -pedantic -g -ggdb -DDEBUG_READ -DDEBUG_WRITE# -Werror
#LFLAGS = `sdl-config --libs` -lSDL2 -lSDL2_ttf -lSDL2_image -lm
LFLAGS = -g -lm
DIRECTORY = bin
#OBJS   = config.o init.o sdlInterface.o graphics.o menu.o gui.o player.o gameObject.o luaLink.o core.o main.o
OBJS   = lodepng.o image.o main.o
PROG = cmbimg
CXX = gcc

# top-level rule to create the program.
all: $(PROG)

# compiling other source files.
%.o: src/%.c src/%.h
	$(CXX) $(CFLAGS) -c -g -ggdb -s $<

# linking the program.
$(PROG): $(OBJS)
	$(CXX) $(OBJS) -g -ggdb -o $(PROG) $(LFLAGS)

# cleaning everything that can be automatically recreated with "make".
clean:
	rm $(OBJS)	

move:
	rm ./bin/$(PROG)
	mv $(PROG) ./bin/$(PROG)
