# automagic makefile with auto dependency generation
# usage:
#      - fill out the needed info (object filesneeded libs CXX flags etc)
#      - run 'make fixbuild' the first time you use the makefile this creates the DEPDIR and OBJDIR directories
#      - run 'make depend'  each time you added a new cpp to the project, after the dependency file is generated it will be kept up-to-date automatically
#      - 'make' will build your program
#      - make clean will delete the object files and the executable
#      - make veryclean will delete all generated files (also core files and *~ and *.bkp)

CPP_OBJECTS_BARE=

C_OBJECTS_BARE=

LIBS=

#either define a progname, or a libname.
PROGNAME=prog
#LIBNAME=
#LIBNAME_BARE=

CC=gcc
CXX=g++
LD=g++
AR=ar

CFLAGS = -Wall -Werror -g
CXXFLAGS = $(CFLAGS)
LDFLAGS = -g
ARFLAGS = rcs

-include presets.mk

RM=rm -f
RMDIR=rm -rf
.PHONY: clean depend veryclean fixbuild

OBJDIR=obj
DEPDIR=dep

# if all you srcfiles are in one subdir, you can list it here to spare you the typing in CPP_OBJECTS_BARE
# otherwise put . here for the current dir
SRCDIR=.

####################################################################################################################################
# no configuration beyond this point
####################################################################################################################################

ifdef LIBNAME_BARE
 ifdef LIBNAME
   echo "CANNOT define *both* LIBNAME and LIBNAME_BARE"
 else
   LIBNAME = $(addprefix lib, $(addsuffix .a, $(LIBNAME_BARE)))
 endif
endif


#create a unique filename from a path
filefrompath=$(subst /,_,$(subst .,_,$(1)))

# how to create various filenames from the bare name
objfile=$(OBJDIR)/$(call filefrompath,$(1)).o
depfile=$(DEPDIR)/$(call filefrompath,$(1)).dep
cppfile=$(SRCDIR)/$(1).cpp
cfile=$(SRCDIR)/$(1).c


# build a rule to create an object from a cpp file, input is the bare filename
define makeobjrule
$(call objfile,$(1)):$(call cppfile,$(1))
	$(CXX) $(CXXFLAGS) -c $$< -o $$@

endef

# build a rule to create an object from a c file, input is the bare filename
define cmakeobjrule
$(call objfile,$(1)):$(call cfile,$(1))
	$(CC) $(CFLAGS) -c $$< -o $$@

endef


#build a rule to create a depfile from a cpp file, bare name is input
define makedeprule
$(call depfile,$(1)):$(call cppfile,$(1))
	$(CXX) $(CXXFLAGS) -c $$< -MM -MF $$@ -MQ $(call objfile,$(1))

endef

#build a rule to create a depfile from a cpp file, bare name is input
define cmakedeprule
$(call depfile,$(1)):$(call cfile,$(1))
	$(CC) $(CFLAGS) -c $$< -MM -MF $$@ -MQ $(call objfile,$(1))

endef


# generate a list of all cpp objectfiles
CPPOBJECTS=$(foreach f,$(CPP_OBJECTS_BARE),$(call objfile,$(f)))

# generate a list of all c objectfiles
COBJECTS=$(foreach f,$(C_OBJECTS_BARE),$(call objfile,$(f)))


# generate a list of all depfile
CPPDEPS=$(foreach f,$(CPP_OBJECTS_BARE),$(call depfile,$(f)))

# generate a list of all depfile
CDEPS=$(foreach f,$(C_OBJECTS_BARE),$(call depfile,$(f)))


# generate rules for all object files
CPPOBJRULES=$(foreach f,$(CPP_OBJECTS_BARE),$(call makeobjrule,$(f)))

# generate rules for all object files
COBJRULES=$(foreach f,$(C_OBJECTS_BARE),$(call cmakeobjrule,$(f)))

# generate rules for all DEP files
CPPDEPRULES=$(foreach f,$(CPP_OBJECTS_BARE),$(call makedeprule,$(f)))

# generate rules for all DEP files
CDEPRULES=$(foreach f,$(C_OBJECTS_BARE),$(call cmakedeprule,$(f)))


ifdef PROGNAME
$(PROGNAME) :fixbuild $(DEPDIR)/depsuptodate $(CPPOBJECTS) $(COBJECTS)
	$(LD) $(LDFLAGS) $(CPPOBJECTS) $(COBJECTS) $(LIBS) -o $(PROGNAME)
endif

ifdef LIBNAME
$(LIBNAME): fixbuild $(DEPDIR)/depsuptodate $(CPPOBJECTS) $(COBJECTS)
	$(AR) $(ARFLAGS) $(LIBNAME) $(CPPOBJECTS) $(COBJECTS)
endif

depend: $(CPPDEPS)  $(CDEPS) fixbuild
#	rm $(DEPDIR)/\*.dep
        
veryclean: clean
	$(RMDIR) dep obj
	$(RM) core core.* *~ *bkp

$(DEPDIR)/depsuptodate: depend makefile fixbuild
	touch $(DEPDIR)/depsuptodate

clean:
	$(RM) $(CPPOBJECTS) $(COBJECTS) $(PROGNAME)

fixbuild: $(DEPDIR) $(OBJDIR)

$(DEPDIR):
	mkdir -p $(DEPDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)


$(eval $(CPPOBJRULES))
$(eval $(CPPDEPRULES))

$(eval $(COBJRULES))
$(eval $(CDEPRULES))


-include $(DEPDIR)/*.dep
