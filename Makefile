CC      := gcc
CFLAGS  := -Wall -Wextra -O2
TARGET  := fscc_config
SRC     := fscc_config.c
SCRIPT  := configure_fscc_range.sh

BIN_INSTALL_DIR := /usr/bin
SH_INSTALL_DIR := /opt/bin

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

install: $(TARGET) $(SCRIPT)
	@echo "Creating install directory..."
	install -d -m 755 $(BIN_INSTALL_DIR)
	install -d -m 755 $(SH_INSTALL_DIR)

	@echo "Installing binary..."
	install -m 775 --owner root --group "Domain Users" $(TARGET) $(BIN_INSTALL_DIR)/

	@echo "Installing script..."
	install -m 775 --owner root --group "Domain Users" $(SCRIPT) $(SH_INSTALL_DIR)/

	@echo "Installation complete."

uninstall:
	@echo "Removing installed files..."
	rm -f $(BIN_INSTALL_DIR)/$(TARGET)
	rm -f $(SH_INSTALL_DIR)/$(SCRIPT)
	@echo "Uninstall complete."