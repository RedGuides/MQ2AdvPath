!include "../global.mak"

ALL : "$(OUTDIR)\MQ2AdvPath.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2AdvPath.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2AdvPath.dll"
	-@erase "$(OUTDIR)\MQ2AdvPath.exp"
	-@erase "$(OUTDIR)\MQ2AdvPath.lib"
	-@erase "$(OUTDIR)\MQ2AdvPath.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2AdvPath.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2AdvPath.dll" /implib:"$(OUTDIR)\MQ2AdvPath.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2AdvPath.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2AdvPath.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2AdvPath.dep")
!INCLUDE "MQ2AdvPath.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2AdvPath.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2AdvPath.cpp

"$(INTDIR)\MQ2AdvPath.obj" : $(SOURCE) "$(INTDIR)"

