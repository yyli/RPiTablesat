BIN = command_fan tsat_pid

all: $(BIN)

command_fan: command_fan.cpp tsat_drivers.cpp
	g++ -g -Wall $? -o $@

tsat_pid: tsat_pid.cpp tsat_drivers.cpp
	g++ -g -Wall $? -o $@

clean:
	rm -rf $(BIN)