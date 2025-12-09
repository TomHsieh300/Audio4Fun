obj-m += tom_dummy_cpu.o
obj-m += tom_dummy_platform.o
obj-m += tom_dummy_codec.o
obj-m += tom_dummy_machine.o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
SIGN_FILE := $(KDIR)/scripts/sign-file
PWD := $(shell pwd)

PRIV_KEY := signing_key.pem
PUB_KEY := signing_key.x509

all:
	make -C $(KDIR) M=$(PWD) modules

sign:
	@echo "--- Signing modules ---"
	@for module in *.ko; do \
		if [ -f "$$module" ]; then \
			echo "Signing $$module..."; \
			$(SIGN_FILE) sha256 $(PRIV_KEY) $(PUB_KEY) "$$module"; \
		fi \
	done
	@echo "--- Signing Done ---"

clean:
	make -C $(KDIR) M=$(PWD) clean
