
SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = .
topdir = $(rubylibdir)/$(arch)
hdrdir = $(rubylibdir)/$(arch)
VPATH = $(srcdir)
prefix = $(DESTDIR)/usr/local
exec_prefix = $(prefix)
sitedir = $(prefix)/lib/ruby/site_ruby
rubylibdir = $(libdir)/ruby/$(ruby_version)
builddir = $(ac_builddir)
archdir = $(rubylibdir)/$(arch)
sbindir = $(exec_prefix)/sbin
compile_dir = $(DESTDIR)/home/ts/ruby/ruby-1.8.0
datadir = $(prefix)/share
includedir = $(prefix)/include
infodir = $(prefix)/info
top_builddir = $(ac_top_builddir)
sysconfdir = $(prefix)/etc
mandir = $(prefix)/man
libdir = $(exec_prefix)/lib
sharedstatedir = $(prefix)/com
oldincludedir = $(DESTDIR)/usr/include
sitearchdir = $(sitelibdir)/$(sitearch)
bindir = $(exec_prefix)/bin
localstatedir = $(prefix)/var
sitelibdir = $(sitedir)/$(ruby_version)
libexecdir = $(exec_prefix)/libexec

CC = gcc
LIBRUBY = $(LIBRUBY_A)
LIBRUBY_A = lib$(RUBY_SO_NAME)-static.a
LIBRUBYARG_SHARED = 
LIBRUBYARG_STATIC = -l$(RUBY_SO_NAME)-static

CFLAGS   = -fPIC -g -O2 -I. -I../src -I/home/ts/local/dbxml/include/dbxml -I/home/ts/local/db4.1.25/include -I/home/ts/local/xercesc/include
CPPFLAGS = -I. -I$(topdir) -I$(hdrdir) -I$(srcdir)  
CXXFLAGS = $(CFLAGS) 
DLDFLAGS = -rdynamic  -Wl,-rpath,/home/ts/local/dbxml/lib -L/home/ts/local/dbxml/lib -ldbxml-1.1  -Wl,-rpath,/home/ts/local/db4.1.25/lib -L/home/ts/local/db4.1.25/lib -ldb-4.1 -ldb_cxx-4.1  -Wl,-rpath,/home/ts/local/pathan/lib -L/home/ts/local/pathan/lib -lpathan  -Wl,-rpath,/home/ts/local/xercesc/lib -L/home/ts/local/xercesc/lib -lxerces-c  
LDSHARED = g++ -shared
AR = ar
EXEEXT = 

RUBY_INSTALL_NAME = ruby
RUBY_SO_NAME = $(RUBY_INSTALL_NAME)
arch = i686-linux
sitearch = i686-linux
ruby_version = 1.8
RUBY = ruby
RM = $(RUBY) -rftools -e "File::rm_f(*ARGV.map do|x|Dir[x]end.flatten.uniq)"
MAKEDIRS = $(RUBY) -r ftools -e 'File::makedirs(*ARGV)'
INSTALL_PROG = $(RUBY) -r ftools -e 'File::install(ARGV[0], ARGV[1], 0755, true)'
INSTALL_DATA = $(RUBY) -r ftools -e 'File::install(ARGV[0], ARGV[1], 0644, true)'

#### End of system configuration section. ####


LIBPATH =  -L"$(libdir)"
DEFFILE = 

CLEANFILES = 
DISTCLEANFILES = 

target_prefix = 
LOCAL_LIBS = 
LIBS =   -ldl -lcrypt -lm  -lc
OBJS = bdbxml.o
TARGET = bdbxml
DLLIB = $(TARGET).so

RUBYCOMMONDIR = $(sitedir)$(target_prefix)
RUBYLIBDIR    = $(sitelibdir)$(target_prefix)
RUBYARCHDIR   = $(sitearchdir)$(target_prefix)

CLEANLIBS     = "$(TARGET).{lib,exp,il?,tds,map}" $(DLLIB)
CLEANOBJS     = "*.{o,a,s[ol],pdb,bak}"

all:		$(DLLIB)

clean:
		@$(RM) $(CLEANLIBS) $(CLEANOBJS) $(CLEANFILES)

distclean:	clean
		@$(RM) Makefile extconf.h conftest.* mkmf.log
		@$(RM) core ruby$(EXEEXT) *~ $(DISTCLEANFILES)

realclean:	distclean
install: $(RUBYARCHDIR)
install: $(RUBYARCHDIR)/$(DLLIB)
$(RUBYARCHDIR)/$(DLLIB): $(DLLIB) $(RUBYARCHDIR)
	@$(INSTALL_PROG) $(DLLIB) $(RUBYARCHDIR)
$(RUBYARCHDIR):
	@$(MAKEDIRS) $(RUBYARCHDIR)

site-install: install

.SUFFIXES: .c .cc .m .cxx .cpp .C .o

.cc.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.cxx.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.C.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(DLLIB): $(OBJS)
	@-$(RM) $@
	$(LDSHARED) $(DLDFLAGS) $(LIBPATH) -o $(DLLIB) $(OBJS) $(LOCAL_LIBS) $(LIBS)


%.html: %.rd
	rd2 $< > ${<:%.rd=%.html}

HTML = bdbxml.html \
	docs/container.html \
	docs/context.html \
	docs/document.html \
	docs/index.html \
	docs/results.html

RDOC = bdbxml.rd \
	docs/container.rd \
	docs/context.rd \
	docs/document.rd \
	docs/index.rd \
	docs/results.rd

rdoc: docs/doc/index.html

docs/doc/index.html: $(RDOC)
	@-(cd docs; ruby b.rb bdbxml; rdoc bdbxml.rb)

rd2: html

html: $(HTML)

test: $(DLLIB)
	ruby tests/xml.rb
