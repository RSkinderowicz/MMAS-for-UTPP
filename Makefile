CXX = g++


mode = release
ifeq ($(mode),release)
	CXXFLAGS = -pipe -std=gnu++14 -Wall -pedantic -O3 -mtune=native -march=native -DNDEBUG
#-g -rdynamic
# -DNDEBUG 
#	-flto
else
# CXXFLAGS = -g $(GOOD_WARN)  -O0  -I /usr/local/cuda-6.5/include/ $(CUDA_GEN_OPT)
	CXXFLAGS = -pg -pipe -std=c++14 -Wall -pedantic -O0 -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self  -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls  -Wsign-conversion -Wsign-promo  -Wstrict-overflow=5 -Wundef -Wno-unused
endif

LDFLAGS = -lpthread -ldl

BUILDDIR = obj
TARGET = ants-tpp
SRCDIR = src
SOURCES = main.cpp\
	  docopt.cpp\
	  logging.cpp\
	  utils.cpp\
	  tpp.cpp\
	  tpp_solution.cpp\
	  gsh.cpp\
	  vec.cpp\
	  two_opt.cpp\
	  three_opt.cpp\
	  drop.cpp\
	  rand.cpp\
	  cah.cpp\
	  ant.cpp\
	  aco.cpp\
	  tpp_info.cpp\
	  basic_pheromone.cpp\
	  stopcondition.cpp

$(shell mkdir -p $(BUILDDIR))

OBJS = $(SOURCES:.cpp=.o)

$(info $$OBJS is [${OBJS}])

OUT_OBJS = $(addprefix $(BUILDDIR)/,$(OBJS))

DEPS = $(SOURCES:%.cpp=$(BUILDDIR)/%.depends)

$(warning $(DEPS))

.PHONY: clean all

all: $(TARGET)

$(TARGET): $(OUT_OBJS)
	$(CXX) $(CXXFLAGS) $(OUT_OBJS) $(LDFLAGS) -o $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.depends: $(SRCDIR)/%.cpp
	$(CXX) -MF"$@" -MG -MM -MP  -MT"$(<F:%.cpp=$(BUILDDIR)/%.o)" $(CXXFLAGS) $< > $@

clean:
	rm -rf $(OUT_OBJS) $(DEPS) $(TARGET) $(BUILDDIR)

-include $(DEPS)
