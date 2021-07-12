# umake
Parse file and execute commands


all : umake

umake.o: umake.c
  gcc -c umake.c
  
umake: umake.o
  gcc -o umake-new umake.omv -i umake-new umake
