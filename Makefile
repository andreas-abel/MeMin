OBJS = DIMACSWriter.o IncSpecSeq.o KISSParser.o MachineBuilder.o minimizer.o
MINISAT_LIB = minisat/core/lib.a
export MROOT = $(CURDIR)/minisat

all: MeMin 

MeMin: $(OBJS) $(MINISAT_LIB)
	g++ $(OBJS) $(MINISAT_LIB) -static -o $@

$(MINISAT_LIB):	
	$(MAKE) -C minisat/core libr

%.o: %.cpp
	g++ -std=c++0x -I./minisat -O3 -Wall -c -fmessage-length=0 -Wno-parentheses -Wno-literal-suffix $< -o $@

clean:
	-$(RM) $(OBJS) MeMin minisat/core/*.a
	$(MAKE) -C minisat/core clean