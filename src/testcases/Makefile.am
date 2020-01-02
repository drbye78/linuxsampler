AM_CPPFLAGS = $(all_includes) $(GIG_CFLAGS) $(SNDFILE_CFLAGS) $(LIB_SF2)
METASOURCES = AUTO

if HAVE_COREMIDI
coremidi_ldflags = -framework CoreFoundation -framework CoreMIDI
else
coremidi_ldflags =
endif

EXTRA_PROGRAMS = linuxsamplertest
linuxsamplertest_SOURCES = \
	linuxsamplertest.cpp \
	NKSPTest.cpp NKSPTest.h \
	PoolTest.cpp PoolTest.h \
	ThreadTest.cpp ThreadTest.h \
	MutexTest.cpp MutexTest.h \
	ConditionTest.cpp ConditionTest.h \
	LSCPTest.cpp LSCPTest.h
linuxsamplertest_LDFLAGS = $(coremidi_ldflags)
linuxsamplertest_LDADD = $(top_builddir)/src/liblinuxsampler.la -lcppunit
