#VERSION=$(shell git tag | sed 's/.*\([0-9]\+\.[0-9]\+\.[0-9]\+\)/\1/' | sort -nt . | tail -1)
# TODO git tags here are not describing versions actually.
VERSION=0.0.1

BIN = wted
LIBS = -lchthon2 -lncurses

SOURCES = $(wildcard *.cpp)
OBJ = $(addprefix tmp/,$(SOURCES:.cpp=.o))
#WARNINGS = -pedantic -Werror -Wall -Wextra -Wformat=2 -Wmissing-include-dirs -Wswitch-default -Wswitch-enum -Wuninitialized -Wunused -Wfloat-equal -Wundef -Wno-endif-labels -Wshadow -Wcast-qual -Wcast-align -Wconversion -Wsign-conversion -Wlogical-op -Wmissing-declarations -Wno-multichar -Wredundant-decls -Wunreachable-code -Winline -Winvalid-pch -Wvla -Wdouble-promotion -Wzero-as-null-pointer-constant -Wuseless-cast -Wvarargs -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wsuggest-attribute=format
CXXFLAGS = -MD -MP -std=c++0x $(WARNINGS)

all: $(BIN)

run: $(BIN)
	screen sh -c './$(BIN) || exec bash'

$(BIN): $(OBJ)
	$(CXX) $(LIBS) -o $@ $^

deb: $(BIN)
	@debpackage.py \
		$(BIN) \
		-v $(VERSION) \
		--maintainer 'umi041 <umi0451@gmail.com>' \
		--bin $(BIN) \
		--build-dir tmp \
		--dest-dir . \
		--description 'Very simple game with ASCII graphics.'

tmp/%.o: %.cpp
	@echo Compiling $<...
	@$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean Makefile

clean:
	$(RM) -rf tmp/* $(BIN)

$(shell mkdir -p tmp)
-include $(OBJ:%.o=%.d)

