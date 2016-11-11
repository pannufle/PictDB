CFLAGS += -std=c99 -g
CFLAGS += -I/usr/local/opt/openssl/include -I./libmongoose
CFLAGS += $$(pkg-config vips --cflags)
LDLIBS += $$(pkg-config vips --libs) -lm -lssl -lcrypto -ljson-c -lmongoose
LDFLAGS += -L./libmongoose/

all : pictDBM pictDB_server

pictDBM : pictDBM.o db_list.o db_utils.o db_create.o error.o db_delete.o image_content.o pictDBM_tools.o dedup.o db_insert.o db_read.o db_gbcollect.o

pictDB_server : pictDB_server.o db_utils.o db_list.o error.o db_utils.o db_read.o image_content.o db_insert.o dedup.o db_delete.o

clean:
	rm -f *.o
	rm -f pictDBM
	rm -f pictDB_server

rebuild: clean all
