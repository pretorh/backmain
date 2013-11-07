CC=gcc
Sources := src/main.c

objs := $(patsubst src/%.c,obj/%.o,$(Sources))

all: obj/ depfile.mk bm

.PHONY: depfile.mk
depfile.mk:
	@$(CC) -MM $(Sources) | sed 's/\(.*\)/obj\/\1/' > $@
-include depfile.mk

clean:
	rm -rf obj/
	rm -f bm depfile.mk

bm: $(objs)
	$(CC) $^ -o $@ -lcrypto

obj/%.o: src/%.c
	$(CC) -ggdb -pedantic -Wall -Wextra --std=gnu99 -c $< -o $@

obj/:
	mkdir $@
