-include nemu/Makefile.git

default:
	@echo "Please run 'make' under any subprojects to compile."
 
clean:
	-$(MAKE) -C nemu clean
	-$(MAKE) -C nexus-am clean
	-$(MAKE) -C nanos-lite clean
	-$(MAKE) -C navy-apps clean

submit: clean
	git gc
	cd .. && tar cj $(shell basename `pwd`) > $(STU_ID).tar.bz2
	
count:
	find nemu -name "*.c" -o -name "*.h" | xargs wc -l | tail -n 1

count-nonblank:
	find nemu -name "*.c" -o -name "*.h" | xargs cat | grep -v '^[[:space:]]*$$' | wc -l | awk '{print $$1, "total"}'

.PHONY: default clean submit count count-nonblank