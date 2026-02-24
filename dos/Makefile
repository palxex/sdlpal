# Makefile for DOS platforms with DJGPP under linux

MAKEFLAGS += -j

TARGET = sdlpal.exe
TEST_TARGET = sdlpal-tests.exe

INTER=.dos

HOST = i586-pc-msdosdjgpp-
TEST_CCFLAGS =

GENERATED = -DPAL_HAS_GIT_REVISION $(shell ../scripts/gengitrev)

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

GTEST_DIR = ../3rd/googletest/googletest

SOURCES = . .. ../sdl_compat ../adplug ../liboggvorbis/src ../libopusfile/src ../libopusfile/celt ../libopusfile/silk ../libopusfile/silk/float ../libmad ../native_midi ../timidity
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))
OBJFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)
DEPFILES = $(CFILES:.c=$(INTER).d) $(CPPFILES:.cpp=$(INTER).d)
TEST_CPPFILES = $(wildcard ../tests/*.cpp)
TEST_OBJFILES = $(TEST_CPPFILES:.cpp=.o)
SDL_CONFIG = sdl2-config

override CCFLAGS += `$(SDL_CONFIG) --cflags` -I../sdl_compat -I../3rd/mingw-std-threads -ggdb -ansi -Wall -Ofast -fno-strict-aliasing $(EXTRA_CCFLAGS) $(TEST_CCFLAGS) $(GENERATED)
CXXFLAGS = $(CCFLAGS) -std=c++11
CFLAGS = $(CCFLAGS) -std=gnu99
OPENGL_LIBS =
LDFLAGS = `$(SDL_CONFIG) --libs` ${OPENGL_LIBS}
TEST_CXXFLAGS += -isystem $(GTEST_DIR)/include -I $(GTEST_DIR) -g -Wall -Wextra -pthread

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

.PHONY : all clean check install mount-vhd unmount-vhd exec-dosbox exec-vm reinstall stop-vm

all: $(TARGET)

VHD_PATH_WIN = D:\Virtual Machines\vbox98\vbox98.vhd
VHD_PATH = /mnt/d/Virtual Machines/vbox98/vbox98.vhd
VHD_TARGET_DIR = PAL

%.dist.exe: %
	@cp -f $< $@
	@strip --strip-unneeded $@
	@upx -9 $@

$(TARGET): $(OBJFILES)
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS)
	@strip --strip-unneeded $@
	@upx -9 $@

install: $(TARGET)
	@echo "关闭虚拟机 vbox98（如果运行中）..."
	@if command -v "/mnt/c/Program Files/Oracle/VirtualBox/VBoxManage.exe" >/dev/null 2>&1; then \
		"/mnt/c/Program Files/Oracle/VirtualBox/VBoxManage.exe" controlvm "vbox98" poweroff 2>/dev/null || true; \
		sleep 2; \
	else \
		echo "⚠ VBoxManage.exe 未找到，跳过"; \
	fi
	@md5sum $(TARGET)
	@if [ -d /mnt/j/pal ]; then \
		cp -f $(TARGET) /mnt/j/pal/$(TARGET) && echo "✓ 已复制到 /mnt/j/pal"; \
	else \
		echo "⚠ /mnt/j/pal 不存在，跳过"; \
	fi
	@echo "正在挂载 VHD..."
	@TEMP_OUT=$$(wslpath -w $$(mktemp)); \
	TARGET_WIN=$$(wslpath -w $$(pwd)/$(TARGET)); \
	SCRIPT_PATH=$$(wslpath -w $$(pwd)/vhd-install.ps1); \
	powershell.exe -Command "gsudo powershell -ExecutionPolicy Bypass -File '$$SCRIPT_PATH' -VhdPath '$(VHD_PATH_WIN)' -SourceFile '$$TARGET_WIN' -TargetDir '$(VHD_TARGET_DIR)' -OutputFile '$$TEMP_OUT'"; \
	VHD_DRIVE=$$(cat "$$(wslpath -u $$TEMP_OUT)" 2>/dev/null | tr -d '\r\n ' | head -c 1); \
	rm -f "$$(wslpath -u $$TEMP_OUT)"; \
	if [ -n "$$VHD_DRIVE" ]; then \
		VHD_DRIVE_LOWER=$$(echo "$$VHD_DRIVE" | tr '[:upper:]' '[:lower:]'); \
		VHD_MOUNT="/mnt/$$VHD_DRIVE_LOWER"; \
		VHD_TARGET="$$VHD_MOUNT/$(VHD_TARGET_DIR)"; \
		echo "✓ VHD 已挂载到驱动器: $$VHD_DRIVE: (保持挂载)"; \
		sleep 1; \
		if [ -f "$$VHD_TARGET/$(TARGET)" ]; then \
			md5sum "$$VHD_TARGET/$(TARGET)"; \
		fi; \
	fi
	@echo "✓ 安装完成"

gtest-all.o : $(GTEST_DIR)/src/gtest-all.cc %.d
	@echo [CC] $<
	@$(HOST)g++ $(DEPFLAGS) $(TEST_CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@$(HOST)gcc $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@$(HOST)g++ $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

$(TEST_TARGET): $(OBJFILES) $(TEST_OBJFILES) gtest-all.o
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS) -pthread

clean:
	-rm -f $(TARGET) $(TEST_TARGET) $(TARGET).dist $(OBJFILES) $(TEST_OBJFILES) $(DEPFILES)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)

../liboggvorbis/%$(INTER).o: EXTRA_CCFLAGS := -I../liboggvorbis/include -I../liboggvorbis/src
../libopusfile/%$(INTER).o: EXTRA_CCFLAGS := -I../liboggvorbis/include -I../libopusfile/include -I../libopusfile/src -I../libopusfile/celt -I../libopusfile/silk -I../libopusfile/silk/float -DHAVE_CONFIG_H -DFLOAT_APPROX
%$(INTER).o: EXTRA_CCFLAGS := -I. -I.. -I../liboggvorbis/include -I../libopusfile/include -I../timidity -DPAL_HAS_PLATFORM_SPECIFIC_UTILS
check: TEST_CCFLAGS = -I. -I.. -DUNIT_TEST=1 -isystem $(GTEST_DIR)/include
check: $(TEST_TARGET)
	@echo [EXEC] $(TEST_TARGET)
	@chmod +x $(TEST_TARGET)
	@exec $(TEST_TARGET)

mount-vhd:
	@echo "关闭虚拟机 vbox98（如果运行中）..."
	@if command -v "/mnt/c/Program Files/Oracle/VirtualBox/VBoxManage.exe" >/dev/null 2>&1; then \
		"/mnt/c/Program Files/Oracle/VirtualBox/VBoxManage.exe" controlvm "vbox98" poweroff 2>/dev/null || true; \
		sleep 2; \
	fi
	@echo "挂载 VHD..."
	@TEMP_OUT=$$(wslpath -w $$(mktemp)); \
	SCRIPT_PATH=$$(wslpath -w $$(pwd)/vhd-mount.ps1); \
	powershell.exe -Command "gsudo powershell -ExecutionPolicy Bypass -File '$$SCRIPT_PATH' -VhdPath '$(VHD_PATH_WIN)' -OutputFile '$$TEMP_OUT'"; \
	VHD_DRIVE=$$(cat "$$(wslpath -u $$TEMP_OUT)" 2>/dev/null | tr -d '\r\n ' | head -c 1); \
	rm -f "$$(wslpath -u $$TEMP_OUT)"; \
	if [ -n "$$VHD_DRIVE" ]; then \
		VHD_DRIVE_LOWER=$$(echo "$$VHD_DRIVE" | tr '[:upper:]' '[:lower:]'); \
		VHD_MOUNT="/mnt/$$VHD_DRIVE_LOWER"; \
		sudo umount -l "$$VHD_MOUNT" 2>/dev/null || true; \
		sudo mkdir -p "$$VHD_MOUNT"; \
		sudo mount -t drvfs "$$VHD_DRIVE:\\" "$$VHD_MOUNT"; \
		echo "✓ 已重新挂载 drvfs: $$VHD_DRIVE: -> $$VHD_MOUNT"; \
	fi

unmount-vhd:
	@echo "卸载 VHD..."
	@SCRIPT_PATH=$$(wslpath -w $$(pwd)/vhd-unmount.ps1); \
	powershell.exe -Command "gsudo powershell -ExecutionPolicy Bypass -File '$$SCRIPT_PATH' -VhdPath '$(VHD_PATH_WIN)'"

exec-dosbox: install
	@echo "启动 DOSBox-X..."
	@/mnt/d/Workspace/dosbox-x/dosbox-x.exe &

exec-vm: install
	@echo "准备启动虚拟机 vbox98..."
	@echo "先卸载 VHD..."
	@SCRIPT_PATH=$$(wslpath -w $$(pwd)/vhd-unmount.ps1); \
	powershell.exe -Command "gsudo powershell -ExecutionPolicy Bypass -File '$$SCRIPT_PATH' -VhdPath '$(VHD_PATH_WIN)'" 2>/dev/null || true
	@sleep 1
	@echo "启动虚拟机..."
	@"/mnt/c/Program Files/Oracle/VirtualBox/VirtualBoxVM.exe" --startvm "vbox98" &

stop-vm:
	@echo "关闭虚拟机 vbox98..."
	@"/mnt/c/Program Files/Oracle/VirtualBox/VBoxManage.exe" controlvm "vbox98" poweroff 2>/dev/null || echo "虚拟机未运行"
	@echo "✓ 虚拟机已关闭"

reinstall: clean install

