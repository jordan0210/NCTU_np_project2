all:
	g++ np_simple.cpp np_simple.h -o np_simple
	g++ np_single_proc.cpp np_single_proc.h -o np_single_proc

clean:
	@rm np_simple
	@rm np_single_proc
