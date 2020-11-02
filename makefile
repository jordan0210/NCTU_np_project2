all:
	g++ np_simple.cpp np_simple.h -o np_simple
	#g++ np_simple_proc.cpp np_simple_proc.h -o np_simple_proc

clean:
	@rm npshell
