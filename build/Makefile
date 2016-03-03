RX_OUT = rxmachine
TX_OUT = txmachine
CH_OUT = channel
CH_SRC = ../src/channel.c ../src/sock.c
RX_SRC = ../src/rxmachine.c ../src/list.c ../src/buffer.c ../src/timer.c ../src/link.c ../src/phy.c ../src/sock.c ../src/slog.c ../src/app_img.c
TX_SRC = ../src/txmachine.c ../src/list.c ../src/buffer.c ../src/timer.c ../src/link.c ../src/phy.c ../src/sock.c ../src/slog.c ../src/app_img.c
all: $(CH_SRC) $(TX_SRC) $(RX_SRC)
	gcc $(CH_SRC) -o $(CH_OUT)
	gcc $(RX_SRC) -pthread -lrt -g -o  $(RX_OUT)
	gcc $(TX_SRC) -pthread -lrt -g -o  $(TX_OUT)
rxm: $(RX_SRC)
	gcc $(RX_SRC) -pthread -lrt -o $(RX_OUT)
txm: $(TX_SRC)
	gcc $(TX_SRC) -pthread -lrt -o $(TX_OUT)
clean:
	rm -f $(CH_OUT) $(TX_OUT) $(RX_OUT)
