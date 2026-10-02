CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = main
SRC = main.c
INPUT_DIR = inputs
INPUT_FILES = input1.txt input2.txt input3.txt
OUTPUT_DIR = outputs

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	@mkdir -p $(OUTPUT_DIR)
	@set -e; \
	for input in $(INPUT_FILES); do \
		echo "Testing $$input"; \
		name=$${input%.txt}; \
		./$(TARGET) "$(INPUT_DIR)/$$input" FCFS 0 > $(OUTPUT_DIR)/$${name}_FCFS.txt; \
		./$(TARGET) "$(INPUT_DIR)/$$input" RR 5 > $(OUTPUT_DIR)/$${name}_RR.txt; \
		./$(TARGET) "$(INPUT_DIR)/$$input" SJF 0 > $(OUTPUT_DIR)/$${name}_SJF.txt; \
	done

clean:
	rm -f $(TARGET)
	rm -rf $(OUTPUT_DIR)
