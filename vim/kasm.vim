" Vim syntax file
" Language:	KASM - The Kestrel Assembler (v0.98)
" Maintainer:	Rakesh Ramesh <rakeshr1@stanford.edu>
" Original Author:	Rakesh Ramesh <rakeshr1@stanford.edu>
" Last Change:	2013 Feb 05
" KASM Home:	http://classes.soe.ucsc.edu/cmpe220/Winter06/docs/kasm_man.pdf

" Setup Syntax:
"  Clear old syntax settings
if version < 600
  syn clear
elseif exists("b:current_syntax")
  finish
endif
"  Assembler syntax is case insensetive
syn case ignore

" Vim search and movement commands on identifers
if version < 600
  "  Comments at start of a line inside which to skip search for indentifiers
  set comments=:;
  "  Identifier Keyword characters (defines \k)
  set iskeyword=@,48-57,#,$,.,?,@-@,_,~
else
  "  Comments at start of a line inside which to skip search for indentifiers
  setlocal comments=:;
  "  Identifier Keyword characters (defines \k)
  setlocal iskeyword=@,48-57,#,$,.,?,@-@,_,~
endif



" Comments:
syn region  kasmComment		start=";" keepend end="$" contains=@kasmGrpInComments
syn region  kasmSpecialComment	start=";\*\*\*" keepend end="$"
syn keyword kasmInCommentTodo	contained TODO FIXME XXX[XXXXX]
syn cluster kasmGrpInComments	contains=kasmInCommentTodo
syn cluster kasmGrpComments	contains=@kasmGrpInComments,kasmComment,kasmSpecialComment



" Label Identifiers:
"  in KASM: 'Everything is a Label'
"  Definition Label = label defined by %[i]define or %[i]assign
"  Identifier Label = label defined as first non-keyword on a line or %[i]macro
syn match   kasmLabelError	"$\=\(\d\+\K\|[#\.@]\|\$\$\k\)\k*\>"
syn match   kasmLabel		"\<\(\h\|[?@]\)\k*\>"
syn match   kasmLabel		"[\$\~]\(\h\|[?@]\)\k*\>"lc=1
"  Labels starting with one or two '.' are special
syn match   kasmLocalLabel	"\<\.\(\w\|[#$?@~]\)\k*\>"
syn match   kasmLocalLabel	"\<\$\.\(\w\|[#$?@~]\)\k*\>"ms=s+1
if !exists("kasm_no_warn")
  syn match  kasmLabelWarn	"\<\~\=\$\=[_\.][_\.\~]*\>"
endif
if exists("kasm_loose_syntax")
  syn match   kasmSpecialLabel	"\<\.\.@\k\+\>"
  syn match   kasmSpecialLabel	"\<\$\.\.@\k\+\>"ms=s+1
  if !exists("kasm_no_warn")
    syn match   kasmLabelWarn	"\<\$\=\.\.@\(\d\|[#$\.~]\)\k*\>"
  endif
  " disallow use of kasm internal label format
  syn match   kasmLabelError	"\<\$\=\.\.@\d\+\.\k*\>"
else
  syn match   kasmSpecialLabel	"\<\.\.@\(\h\|[?@]\)\k*\>"
  syn match   kasmSpecialLabel	"\<\$\.\.@\(\h\|[?@]\)\k*\>"ms=s+1
endif
"  Labels can be dereferenced with '$' to destinguish them from reserved words
syn match   kasmLabelError	"\<\$\K\k*\s*:"
syn match   kasmLabelError	"^\s*\$\K\k*\>"
syn match   kasmLabelError	"\<\~\s*\(\k*\s*:\|\$\=\.\k*\)"



" Constants:
syn match   kasmStringError	+["']+
syn match   kasmString		+\("[^"]\{-}"\|'[^']\{-}'\)+
syn match   kasmBinNumber	"\<[0-1]\+b\>"
syn match   kasmBinNumber	"\<\~[0-1]\+b\>"lc=1
syn match   kasmOctNumber	"\<\o\+q\>"
syn match   kasmOctNumber	"\<\~\o\+q\>"lc=1
syn match   kasmDecNumber	"\<\d\+\>"
syn match   kasmDecNumber	"\<\~\d\+\>"lc=1
syn match   kasmHexNumber	"\<\(\d\x*h\|0x\x\+\|\$\d\x*\)\>"
syn match   kasmHexNumber	"\<\~\(\d\x*h\|0x\x\+\|\$\d\x*\)\>"lc=1
syn match   kasmFltNumber	"\<\d\+\.\d*\(e[+-]\=\d\+\)\=\>"
syn keyword kasmFltNumber	Inf Infinity Indefinite NaN SNaN QNaN
syn match   kasmNumberError	"\<\~\s*\d\+\.\d*\(e[+-]\=\d\+\)\=\>"



" Netwide Assembler Storage Directives:
"  Storage types
syn keyword kasmTypeError	DF EXTRN FWORD RESF TBYTE
syn keyword kasmType		FAR NEAR SHORT
syn keyword kasmType		BYTE WORD DWORD QWORD DQWORD HWORD DHWORD TWORD
syn keyword kasmType		CDECL FASTCALL NONE PASCAL STDCALL
syn keyword kasmStorage		DB DW DD DQ DDQ DT
syn keyword kasmStorage		RESB RESW RESD RESQ RESDQ REST
syn keyword kasmStorage		EXTERN GLOBAL COMMON
"  Structured storage types
" syn match   kasmTypeError	"\<\(AT\|I\=\(END\)\=\(STRUCT\=\|UNION\)\|I\=END\)\>"
" syn match   kasmStructureLabel	contained "\<\(AT\|I\=\(END\)\=\(STRUCT\=\|UNION\)\|I\=END\)\>"
"   structures cannot be nested (yet) -> use: 'keepend' and 're='
syn cluster kasmGrpCntnStruc	contains=ALLBUT,@kasmGrpInComments,kasmMacroDef,@kasmGrpInMacros,@kasmGrpInPreCondits,kasmStructureDef,@kasmGrpInStrucs
syn region  kasmStructureDef	transparent matchgroup=kasmStructure keepend start="^\s*STRUCT\>"hs=e-5 end="^\s*ENDSTRUCT\>"re=e-9 contains=@kasmGrpCntnStruc
syn region  kasmStructureDef	transparent matchgroup=kasmStructure keepend start="^\s*STRUC\>"hs=e-4  end="^\s*ENDSTRUC\>"re=e-8  contains=@kasmGrpCntnStruc
syn region  kasmStructureDef	transparent matchgroup=kasmStructure keepend start="\<ISTRUCT\=\>" end="\<IEND\(STRUCT\=\)\=\>" contains=@kasmGrpCntnStruc,kasmInStructure
"   union types are not part of kasm (yet)
"syn region  kasmStructureDef	transparent matchgroup=kasmStructure keepend start="^\s*UNION\>"hs=e-4 end="^\s*ENDUNION\>"re=e-8 contains=@kasmGrpCntnStruc
"syn region  kasmStructureDef	transparent matchgroup=kasmStructure keepend start="\<IUNION\>" end="\<IEND\(UNION\)\=\>" contains=@kasmGrpCntnStruc,kasmInStructure
syn match   kasmInStructure	contained "^\s*AT\>"hs=e-1
syn cluster kasmGrpInStrucs	contains=kasmStructure,kasmInStructure,kasmStructureLabel



" PreProcessor Instructions:
" NAsm PreProcs start with %, but % is not a character
syn match   kasmPreProcError	"%{\=\(%\=\k\+\|%%\+\k*\|[+-]\=\d\+\)}\="
if exists("kasm_loose_syntax")
  syn cluster kasmGrpNxtCtx	contains=kasmStructureLabel,kasmLabel,kasmLocalLabel,kasmSpecialLabel,kasmLabelError,kasmPreProcError
else
  syn cluster kasmGrpNxtCtx	contains=kasmStructureLabel,kasmLabel,kasmLabelError,kasmPreProcError
endif

"  Multi-line macro
syn cluster kasmGrpCntnMacro	contains=ALLBUT,@kasmGrpInComments,kasmStructureDef,@kasmGrpInStrucs,kasmMacroDef,@kasmGrpPreCondits,kasmMemReference,kasmInMacPreCondit,kasmInMacStrucDef
syn region  kasmMacroDef	matchgroup=kasmMacro keepend start="^\s*%macrodef\>"hs=e-5 start="^\s*%imacro\>"hs=e-6 end="^\s*%macroend\>"re=e-9 contains=@kasmGrpCntnMacro,kasmInMacStrucDef
if exists("kasm_loose_syntax")
  syn match  kasmInMacLabel	contained "%\(%\k\+\>\|{%\k\+}\)"
  syn match  kasmInMacLabel	contained "%\($\+\(\w\|[#\.?@~]\)\k*\>\|{$\+\(\w\|[#\.?@~]\)\k*}\)"
  syn match  kasmInMacPreProc	contained "^\s*%\(push\|repl\)\>"hs=e-4 skipwhite nextgroup=kasmStructureLabel,kasmLabel,kasmInMacParam,kasmLocalLabel,kasmSpecialLabel,kasmLabelError,kasmPreProcError
  if !exists("kasm_no_warn")
    syn match kasmInMacLblWarn	contained "%\(%[$\.]\k*\>\|{%[$\.]\k*}\)"
    syn match kasmInMacLblWarn	contained "%\($\+\(\d\|[#\.@~]\)\k*\|{\$\+\(\d\|[#\.@~]\)\k*}\)"
    hi link kasmInMacCatLabel	kasmInMacLblWarn
  else
    hi link kasmInMacCatLabel	kasmInMacLabel
  endif
else
  syn match  kasmInMacLabel	contained "%\(%\(\w\|[#?@~]\)\k*\>\|{%\(\w\|[#?@~]\)\k*}\)"
  syn match  kasmInMacLabel	contained "%\($\+\(\h\|[?@]\)\k*\>\|{$\+\(\h\|[?@]\)\k*}\)"
  hi link kasmInMacCatLabel	kasmLabelError
endif
syn match   kasmInMacCatLabel	contained "\d\K\k*"lc=1
syn match   kasmInMacLabel	contained "\d}\k\+"lc=2
if !exists("kasm_no_warn")
  syn match  kasmInMacLblWarn	contained "%\(\($\+\|%\)[_~][._~]*\>\|{\($\+\|%\)[_~][._~]*}\)"
endif
syn match   kasmInMacPreProc	contained "^\s*%pop\>"hs=e-3
syn match   kasmInMacPreProc	contained "^\s*%\(push\|repl\)\>"hs=e-4 skipwhite nextgroup=@kasmGrpNxtCtx
"   structures cannot be nested (yet) -> use: 'keepend' and 're='
syn region  kasmInMacStrucDef	contained transparent matchgroup=kasmStructure keepend start="^\s*STRUCT\>"hs=e-5 end="^\s*ENDSTRUCT\>"re=e-9 contains=@kasmGrpCntnMacro
syn region  kasmInMacStrucDef	contained transparent matchgroup=kasmStructure keepend start="^\s*STRUC\>"hs=e-4  end="^\s*ENDSTRUC\>"re=e-8  contains=@kasmGrpCntnMacro
syn region  kasmInMacStrucDef	contained transparent matchgroup=kasmStructure keepend start="\<ISTRUCT\=\>" end="\<IEND\(STRUCT\=\)\=\>" contains=@kasmGrpCntnMacro,kasmInStructure
"   union types are not part of kasm (yet)
"syn region  kasmInMacStrucDef	contained transparent matchgroup=kasmStructure keepend start="^\s*UNION\>"hs=e-4 end="^\s*ENDUNION\>"re=e-8 contains=@kasmGrpCntnMacro
"syn region  kasmInMacStrucDef	contained transparent matchgroup=kasmStructure keepend start="\<IUNION\>" end="\<IEND\(UNION\)\=\>" contains=@kasmGrpCntnMacro,kasmInStructure
syn region  kasmInMacPreConDef	contained transparent matchgroup=kasmInMacPreCondit start="^\s*%ifnidni\>"hs=e-7 start="^\s*%if\(idni\|n\(ctx\|def\|idn\|num\|str\)\)\>"hs=e-6 start="^\s*%if\(ctx\|def\|idn\|nid\|num\|str\)\>"hs=e-5 start="^\s*%ifid\>"hs=e-4 start="^\s*%if\>"hs=e-2 end="%endif\>" contains=@kasmGrpCntnMacro,kasmInMacPreCondit,kasmInPreCondit
" Todo: allow STRUC/ISTRUC to be used inside preprocessor conditional block
syn match   kasmInMacPreCondit	contained transparent "ctx\s"lc=3 skipwhite nextgroup=@kasmGrpNxtCtx
syn match   kasmInMacPreCondit	contained "^\s*%elifctx\>"hs=e-7 skipwhite nextgroup=@kasmGrpNxtCtx
syn match   kasmInMacPreCondit	contained "^\s*%elifnctx\>"hs=e-8 skipwhite nextgroup=@kasmGrpNxtCtx
syn match   kasmInMacParamNum	contained "\<\d\+\.list\>"me=e-5
syn match   kasmInMacParamNum	contained "\<\d\+\.nolist\>"me=e-7
syn match   kasmInMacDirective	contained "\.\(no\)\=list\>"
syn match   kasmInMacMacro	contained transparent "macrodef\s"lc=5 skipwhite nextgroup=kasmStructureLabel
syn match   kasmInMacMacro	contained "^\s*%rotate\>"hs=e-6
syn match   kasmInMacParam	contained "%\([+-]\=\d\+\|{[+-]\=\d\+}\)"
"   kasm conditional macro operands/arguments
"   Todo: check feasebility; add too kasmGrpInMacros, etc.
"syn match   kasmInMacCond	contained "\<\(N\=\([ABGL]E\=\|[CEOSZ]\)\|P[EO]\=\)\>"
syn cluster kasmGrpInMacros	contains=kasmMacro,kasmInMacMacro,kasmInMacParam,kasmInMacParamNum,kasmInMacDirective,kasmInMacLabel,kasmInMacLblWarn,kasmInMacMemRef,kasmInMacPreConDef,kasmInMacPreCondit,kasmInMacPreProc,kasmInMacStrucDef

"   Context pre-procs that are better used inside a macro
if exists("kasm_ctx_outside_macro")
  syn region kasmPreConditDef	transparent matchgroup=kasmCtxPreCondit start="^\s*%ifnctx\>"hs=e-6 start="^\s*%ifctx\>"hs=e-5 end="%endif\>" contains=@kasmGrpCntnPreCon
  syn match  kasmCtxPreProc	"^\s*%pop\>"hs=e-3
  if exists("kasm_loose_syntax")
    syn match   kasmCtxLocLabel	"%$\+\(\w\|[#\.?@~]\)\k*\>"
  else
    syn match   kasmCtxLocLabel	"%$\+\(\h\|[?@]\)\k*\>"
  endif
  syn match kasmCtxPreProc	"^\s*%\(push\|repl\)\>"hs=e-4 skipwhite nextgroup=@kasmGrpNxtCtx
  syn match kasmCtxPreCondit	contained transparent "ctx\s"lc=3 skipwhite nextgroup=@kasmGrpNxtCtx
  syn match kasmCtxPreCondit	contained "^\s*%elifctx\>"hs=e-7 skipwhite nextgroup=@kasmGrpNxtCtx
  syn match kasmCtxPreCondit	contained "^\s*%elifnctx\>"hs=e-8 skipwhite nextgroup=@kasmGrpNxtCtx
  if exists("kasm_no_warn")
    hi link kasmCtxPreCondit	kasmPreCondit
    hi link kasmCtxPreProc	kasmPreProc
    hi link kasmCtxLocLabel	kasmLocalLabel
  else
    hi link kasmCtxPreCondit	kasmPreProcWarn
    hi link kasmCtxPreProc	kasmPreProcWarn
    hi link kasmCtxLocLabel	kasmLabelWarn
  endif
endif

"  Conditional assembly
syn cluster kasmGrpCntnPreCon	contains=ALLBUT,@kasmGrpInComments,@kasmGrpInMacros,@kasmGrpInStrucs
syn region  kasmPreConditDef	transparent matchgroup=kasmPreCondit start="^\s*%ifnidni\>"hs=e-7 start="^\s*%if\(idni\|n\(def\|idn\|num\|str\)\)\>"hs=e-6 start="^\s*%if\(def\|idn\|nid\|num\|str\)\>"hs=e-5 start="^\s*%ifid\>"hs=e-4 start="^\s*%if\>"hs=e-2 end="%endif\>" contains=@kasmGrpCntnPreCon
syn match   kasmInPreCondit	contained "^\s*%el\(if\|se\)\>"hs=e-4
syn match   kasmInPreCondit	contained "^\s*%elifid\>"hs=e-6
syn match   kasmInPreCondit	contained "^\s*%elif\(def\|idn\|nid\|num\|str\)\>"hs=e-7
syn match   kasmInPreCondit	contained "^\s*%elif\(n\(def\|idn\|num\|str\)\|idni\)\>"hs=e-8
syn match   kasmInPreCondit	contained "^\s*%elifnidni\>"hs=e-9
syn cluster kasmGrpInPreCondits	contains=kasmPreCondit,kasmInPreCondit,kasmCtxPreCondit
syn cluster kasmGrpPreCondits	contains=kasmPreConditDef,@kasmGrpInPreCondits,kasmCtxPreProc,kasmCtxLocLabel

"  Other pre-processor statements
syn match   kasmPreProc		"^\s*%\(rep\|use\)\>"hs=e-3
syn match   kasmPreProc		"^\s*%line\>"hs=e-4
syn match   kasmPreProc		"^\s*%\(clear\|error\|fatal\)\>"hs=e-5
syn match   kasmPreProc		"^\s*%\(endrep\|strlen\|substr\)\>"hs=e-6
syn match   kasmPreProc		"^\s*%\(exitrep\|warning\)\>"hs=e-7
syn match   kasmDefine		"^\s*%undef\>"hs=e-5
syn match   kasmDefine		"^\s*%\(assign\|define\)\>"hs=e-6
syn match   kasmDefine		"^\s*%i\(assign\|define\)\>"hs=e-7
syn match   kasmDefine		"^\s*%unmacro\>"hs=e-7
syn match   kasmInclude		"^\s*%include\>"hs=e-7
" Todo: Treat the line tail after %fatal, %error, %warning as text

"  Multiple pre-processor instructions on single line detection (obsolete)
"syn match   kasmPreProcError	+^\s*\([^\t "%';][^"%';]*\|[^\t "';][^"%';]\+\)%\a\+\>+
syn cluster kasmGrpPreProcs	contains=kasmMacroDef,@kasmGrpInMacros,@kasmGrpPreCondits,kasmPreProc,kasmDefine,kasmInclude,kasmPreProcWarn,kasmPreProcError



" Register Identifiers:
"  Register operands:
"  Rakesh : Fix general purpose registers for KASM
" syn match   kasmGen08Register	"\<[A-D][HL]\>"
syn match   kasmGen08Register	"\<[LR][0-9]\|[LR][0-9][0-9]\>"
syn match   kasmGen16Register	"\<\([A-D]X\|[DS]I\|[BS]P\)\>"
syn match   kasmGen32Register	"\<E\([A-D]X\|[DS]I\|[BS]P\)\>"
syn match   kasmGen64Register	"\<R\([A-D]X\|[DS]I\|[BS]P\|[89]\|1[0-5]\|[89][WD]\|1[0-5][WD]\)\>"
syn match   kasmSegRegister	"\<[C-GS]S\>"
syn match   kasmSpcRegister	"\<E\=IP\>"
syn match   kasmFpuRegister	"\<ST\o\>"
syn match   kasmMmxRegister	"\<MM\o\>"
syn match   kasmSseRegister	"\<XMM\o\>"
syn match   kasmCtrlRegister	"\<CR\o\>"
syn match   kasmDebugRegister	"\<DR\o\>"
syn match   kasmTestRegister	"\<TR\o\>"
syn match   kasmRegisterError	"\<\(CR[15-9]\|DR[4-58-9]\|TR[0-28-9]\)\>"
syn match   kasmRegisterError	"\<X\=MM[8-9]\>"
syn match   kasmRegisterError	"\<ST\((\d)\|[8-9]\>\)"
syn match   kasmRegisterError	"\<E\([A-D][HL]\|[C-GS]S\)\>"
"  Memory reference operand (address):
syn match   kasmMemRefError	"[\[\]]"
syn cluster kasmGrpCntnMemRef	contains=ALLBUT,@kasmGrpComments,@kasmGrpPreProcs,@kasmGrpInStrucs,kasmMemReference,kasmMemRefError
syn match   kasmInMacMemRef	contained "\[[^;\[\]]\{-}\]" contains=@kasmGrpCntnMemRef,kasmPreProcError,kasmInMacLabel,kasmInMacLblWarn,kasmInMacParam
syn match   kasmMemReference	"\[[^;\[\]]\{-}\]" contains=@kasmGrpCntnMemRef,kasmPreProcError,kasmCtxLocLabel



" Netwide Assembler Directives:
"  Compilation constants
syn keyword kasmConstant	__BITS__ __DATE__ __FILE__ __FORMAT__ __LINE__
syn keyword kasmConstant	__KASM_MAJOR__ __KASM_MINOR__ __KASM_VERSION__
syn keyword kasmConstant	__TIME__
"  Instruction modifiers
syn match   kasmInstructnError	"\<TO\>"
syn match   kasmInstrModifier	"\(^\|:\)\s*[C-GS]S\>"ms=e-1
syn keyword kasmInstrModifier	A16 A32 O16 O32
syn match   kasmInstrModifier	"\<F\(ADD\|MUL\|\(DIV\|SUB\)R\=\)\s\+TO\>"lc=5,ms=e-1
"   the 'to' keyword is not allowed for fpu-pop instructions (yet)
"syn match   kasmInstrModifier	"\<F\(ADD\|MUL\|\(DIV\|SUB\)R\=\)P\s\+TO\>"lc=6,ms=e-1
"  NAsm directives
syn keyword kasmRepeat		TIMES
syn keyword kasmDirective	ALIGN[B] INCBIN EQU NOSPLIT SPLIT
syn keyword kasmDirective	ABSOLUTE BITS SECTION SEGMENT
syn keyword kasmDirective	ENDSECTION ENDSEGMENT
syn keyword kasmDirective   ENDCOND BEGINCOND DEFINE
syn keyword kasmDirective   START END
syn keyword kasmDirective	__SECT__
"  Macro created standard directives: (requires %include)
syn case match
syn keyword kasmStdDirective	ENDPROC EPILOGUE LOCALS PROC PROLOGUE USES
syn keyword kasmStdDirective	ENDIF ELSE ELIF ELSIF IF
"syn keyword kasmStdDirective	BREAK CASE DEFAULT ENDSWITCH SWITCH
"syn keyword kasmStdDirective	CASE OF ENDCASE
syn keyword kasmStdDirective	DO ENDFOR ENDWHILE FOR REPEAT UNTIL WHILE EXIT
syn case ignore
"  Format specific directives: (all formats)
"  (excluded: extension directives to section, global, common and extern)
syn keyword kasmFmtDirective	ORG
syn keyword kasmFmtDirective	EXPORT IMPORT GROUP UPPERCASE SEG WRT
syn keyword kasmFmtDirective	LIBRARY
syn case match
syn keyword kasmFmtDirective	_GLOBAL_OFFSET_TABLE_ __GLOBAL_OFFSET_TABLE_
syn keyword kasmFmtDirective	..start ..got ..gotoff ..gotpc ..plt ..sym
syn case ignore



" Standard Instructions:
" Rakesh : Fix the KASM instructions
syn match   kasmInstructnError	"\<\(F\=CMOV\|SET\)N\=\a\{0,2}\>"
syn keyword kasmInstructnError	CMPS MOVS LCS LODS STOS XLAT
syn match   kasmStdInstruction	"\<MOVE\>"
syn match   kasmInstructnError	"\<MOV\s[^,;[]*\<CS\>\s*[^:]"he=e-1
syn match   kasmStdInstruction	"\<\(CMOV\|J\|SET\)\(N\=\([ABGL]E\=\|[CEOSZ]\)\|P[EO]\=\)\>"
syn match   kasmStdInstruction	"\<POP\>"
syn keyword kasmStdInstruction	AAA AAD AAM AAS ADC ADD AND
syn keyword kasmStdInstruction  MAXC MINC EQUALC
syn keyword kasmStdInstruction	BOUND BSF BSR BSWAP BT[C] BTR BTS
syn keyword kasmStdInstruction	CALL CBW CDQ CLC CLD CMC CMP CMPSB CMPSD CMPSW CMPSQ
syn keyword kasmStdInstruction	CMPXCHG CMPXCHG8B CPUID CWD[E] CQO
syn keyword kasmStdInstruction	DAA DAS DEC DIV ENTER
syn keyword kasmStdInstruction	IDIV IMUL INC INT[O] IRET[D] IRETW IRETQ
syn keyword kasmStdInstruction	JCXZ JECXZ JMP
syn keyword kasmStdInstruction	LAHF LDS LEA LEAVE LES LFS LGS LODSB LODSD LODSQ
syn keyword kasmStdInstruction	LODSW LOOP[E] LOOPNE LOOPNZ LOOPZ LSS
syn keyword kasmStdInstruction  BEGINLOOP ENDLOOP MACRODEF MACROEND
syn keyword kasmStdInstruction	MOVSB MOVSD MOVSW MOVSX MOVSQ MOVZX MUL NEG NOP NOT
syn keyword kasmStdInstruction	OR POPA[D] POPAW POPF[D] POPFW POPFQ
syn keyword kasmStdInstruction	PUSH[AD] PUSHAW PUSHF[D] PUSHFW PUSHFQ
syn keyword kasmStdInstruction  BSPUSH BSPOP BSNOT BSCLEARM
syn keyword kasmStdInstruction	RCL RCR RETF RET[N] ROL ROR
syn keyword kasmStdInstruction	SAHF SAL SAR SBB SCASB SCASD SCASW
syn keyword kasmStdInstruction	SHL[D] SHR[D] STC STD STOSB STOSD STOSW STOSQ SUB
syn keyword kasmStdInstruction	TEST XADD XCHG XLATB XOR
syn keyword kasmStdInstruction	LFENCE MFENCE SFENCE


" System Instructions: (usually privileged)
"  Verification of pointer parameters
syn keyword kasmSysInstruction	ARPL LAR LSL VERR VERW
"  Addressing descriptor tables
syn keyword kasmSysInstruction	LLDT SLDT LGDT SGDT
"  Multitasking
syn keyword kasmSysInstruction	LTR STR
"  Coprocessing and Multiprocessing (requires fpu and multiple cpu's resp.)
syn keyword kasmSysInstruction	CLTS LOCK WAIT
"  Input and Output
syn keyword kasmInstructnError	INS OUTS
syn keyword kasmSysInstruction	IN INSB INSW INSD OUT OUTSB OUTSB OUTSW OUTSD
"  Interrupt control
syn keyword kasmSysInstruction	CLI STI LIDT SIDT
"  System control
syn match   kasmSysInstruction	"\<MOV\s[^;]\{-}\<CR\o\>"me=s+3
syn keyword kasmSysInstruction	HLT INVD LMSW
syn keyword kasmSseInstruction	PREFETCHT0 PREFETCHT1 PREFETCHT2 PREFETCHNTA
syn keyword kasmSseInstruction	RSM SFENCE SMSW SYSENTER SYSEXIT UD2 WBINVD
"  TLB (Translation Lookahead Buffer) testing
syn match   kasmSysInstruction	"\<MOV\s[^;]\{-}\<TR\o\>"me=s+3
syn keyword kasmSysInstruction	INVLPG

" Debugging Instructions: (privileged)
syn match   kasmDbgInstruction	"\<MOV\s[^;]\{-}\<DR\o\>"me=s+3
syn keyword kasmDbgInstruction	INT1 INT3 RDMSR RDTSC RDPMC WRMSR


" Floating Point Instructions: (requires FPU)
syn match   kasmFpuInstruction	"\<FCMOVN\=\([AB]E\=\|[CEPUZ]\)\>"
syn keyword kasmFpuInstruction	F2XM1 FABS FADD[P] FBLD FBSTP
syn keyword kasmFpuInstruction	FCHS FCLEX FCOM[IP] FCOMP[P] FCOS
syn keyword kasmFpuInstruction	FDECSTP FDISI FDIV[P] FDIVR[P] FENI FFREE
syn keyword kasmFpuInstruction	FIADD FICOM[P] FIDIV[R] FILD
syn keyword kasmFpuInstruction	FIMUL FINCSTP FINIT FIST[P] FISUB[R]
syn keyword kasmFpuInstruction	FLD[1] FLDCW FLDENV FLDL2E FLDL2T FLDLG2
syn keyword kasmFpuInstruction	FLDLN2 FLDPI FLDZ FMUL[P]
syn keyword kasmFpuInstruction	FNCLEX FNDISI FNENI FNINIT FNOP FNSAVE
syn keyword kasmFpuInstruction	FNSTCW FNSTENV FNSTSW FNSTSW
syn keyword kasmFpuInstruction	FPATAN FPREM[1] FPTAN FRNDINT FRSTOR
syn keyword kasmFpuInstruction	FSAVE FSCALE FSETPM FSIN FSINCOS FSQRT
syn keyword kasmFpuInstruction	FSTCW FSTENV FST[P] FSTSW FSUB[P] FSUBR[P]
syn keyword kasmFpuInstruction	FTST FUCOM[IP] FUCOMP[P]
syn keyword kasmFpuInstruction	FXAM FXCH FXTRACT FYL2X FYL2XP1


" Multi Media Xtension Packed Instructions: (requires MMX unit)
"  Standard MMX instructions: (requires MMX1 unit)
syn match   kasmInstructnError	"\<P\(ADD\|SUB\)U\=S\=[DQ]\=\>"
syn match   kasmInstructnError	"\<PCMP\a\{0,2}[BDWQ]\=\>"
syn keyword kasmMmxInstruction	EMMS MOVD MOVQ
syn keyword kasmMmxInstruction	PACKSSDW PACKSSWB PACKUSWB PADDB PADDD PADDW
syn keyword kasmMmxInstruction	PADDSB PADDSW PADDUSB PADDUSW PAND[N]
syn keyword kasmMmxInstruction	PCMPEQB PCMPEQD PCMPEQW PCMPGTB PCMPGTD PCMPGTW
syn keyword kasmMmxInstruction	PMACHRIW PMADDWD PMULHW PMULLW POR
syn keyword kasmMmxInstruction	PSLLD PSLLQ PSLLW PSRAD PSRAW PSRLD PSRLQ PSRLW
syn keyword kasmMmxInstruction	PSUBB PSUBD PSUBW PSUBSB PSUBSW PSUBUSB PSUBUSW
syn keyword kasmMmxInstruction	PUNPCKHBW PUNPCKHDQ PUNPCKHWD
syn keyword kasmMmxInstruction	PUNPCKLBW PUNPCKLDQ PUNPCKLWD PXOR
"  Extended MMX instructions: (requires MMX2/SSE unit)
syn keyword kasmMmxInstruction	MASKMOVQ MOVNTQ
syn keyword kasmMmxInstruction	PAVGB PAVGW PEXTRW PINSRW PMAXSW PMAXUB
syn keyword kasmMmxInstruction	PMINSW PMINUB PMOVMSKB PMULHUW PSADBW PSHUFW


" Streaming SIMD Extension Packed Instructions: (requires SSE unit)
syn match   kasmInstructnError	"\<CMP\a\{1,5}[PS]S\>"
syn match   kasmSseInstruction	"\<CMP\(N\=\(EQ\|L[ET]\)\|\(UN\)\=ORD\)\=[PS]S\>"
syn keyword kasmSseInstruction	ADDPS ADDSS ANDNPS ANDPS
syn keyword kasmSseInstruction	COMISS CVTPI2PS CVTPS2PI
syn keyword kasmSseInstruction	CVTSI2SS CVTSS2SI CVTTPS2PI CVTTSS2SI
syn keyword kasmSseInstruction	DIVPS DIVSS FXRSTOR FXSAVE LDMXCSR
syn keyword kasmSseInstruction	MAXPS MAXSS MINPS MINSS MOVAPS MOVHLPS MOVHPS
syn keyword kasmSseInstruction	MOVLHPS MOVLPS MOVMSKPS MOVNTPS MOVSS MOVUPS
syn keyword kasmSseInstruction	MULPS MULSS
syn keyword kasmSseInstruction	ORPS RCPPS RCPSS RSQRTPS RSQRTSS
syn keyword kasmSseInstruction	SHUFPS SQRTPS SQRTSS STMXCSR SUBPS SUBSS
syn keyword kasmSseInstruction	UCOMISS UNPCKHPS UNPCKLPS XORPS


" Three Dimensional Now Packed Instructions: (requires 3DNow! unit)
syn keyword kasmNowInstruction	FEMMS PAVGUSB PF2ID PFACC PFADD PFCMPEQ PFCMPGE
syn keyword kasmNowInstruction	PFCMPGT PFMAX PFMIN PFMUL PFRCP PFRCPIT1
syn keyword kasmNowInstruction	PFRCPIT2 PFRSQIT1 PFRSQRT PFSUB[R] PI2FD
syn keyword kasmNowInstruction	PMULHRWA PREFETCH[W]


" Vendor Specific Instructions:
"  Cyrix instructions (requires Cyrix processor)
syn keyword kasmCrxInstruction	PADDSIW PAVEB PDISTIB PMAGW PMULHRW[C] PMULHRIW
syn keyword kasmCrxInstruction	PMVGEZB PMVLZB PMVNZB PMVZB PSUBSIW
syn keyword kasmCrxInstruction	RDSHR RSDC RSLDT SMINT SMINTOLD SVDC SVLDT SVTS
syn keyword kasmCrxInstruction	WRSHR
"  AMD instructions (requires AMD processor)
syn keyword kasmAmdInstruction	SYSCALL SYSRET


" Undocumented Instructions:
syn match   kasmUndInstruction	"\<POP\s[^;]*\<CS\>"me=s+3
syn keyword kasmUndInstruction	CMPXCHG486 IBTS ICEBP INT01 INT03 LOADALL
syn keyword kasmUndInstruction	LOADALL286 LOADALL386 SALC SMI UD1 UMOV XBTS



" Synchronize Syntax:
syn sync clear
syn sync minlines=50		"for multiple region nesting
syn sync match  kasmSync	grouphere kasmMacroDef "^\s*%i\=macrodef\>"me=s-1
syn sync match	kasmSync	grouphere NONE	       "^\s*%macroend\>"


" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later  : only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_kasm_syntax_inits")
  if version < 508
    let did_kasm_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " Sub Links:
  HiLink kasmInMacDirective	kasmDirective
  HiLink kasmInMacLabel		kasmLocalLabel
  HiLink kasmInMacLblWarn	kasmLabelWarn
  HiLink kasmInMacMacro		kasmMacro
  HiLink kasmInMacParam		kasmMacro
  HiLink kasmInMacParamNum	kasmDecNumber
  HiLink kasmInMacPreCondit	kasmPreCondit
  HiLink kasmInMacPreProc	kasmPreProc
  HiLink kasmInPreCondit	kasmPreCondit
  HiLink kasmInStructure	kasmStructure
  HiLink kasmStructureLabel	kasmStructure

  " Comment Group:
  HiLink kasmComment		Comment
  HiLink kasmSpecialComment	SpecialComment
  HiLink kasmInCommentTodo	Todo

  " Constant Group:
  HiLink kasmString		String
  HiLink kasmStringError	Error
  HiLink kasmBinNumber		Number
  HiLink kasmOctNumber		Number
  HiLink kasmDecNumber		Number
  HiLink kasmHexNumber		Number
  HiLink kasmFltNumber		Float
  HiLink kasmNumberError	Error

  " Identifier Group:
  HiLink kasmLabel		Identifier
  HiLink kasmLocalLabel		Identifier
  HiLink kasmSpecialLabel	Special
  HiLink kasmLabelError		Error
  HiLink kasmLabelWarn		Todo

  " PreProc Group:
  HiLink kasmPreProc		PreProc
  HiLink kasmDefine		Define
  HiLink kasmInclude		Include
  HiLink kasmMacro		Macro
  HiLink kasmPreCondit		PreCondit
  HiLink kasmPreProcError	Error
  HiLink kasmPreProcWarn	Todo

  " Type Group:
  HiLink kasmType		Type
  HiLink kasmStorage		StorageClass
  HiLink kasmStructure		Structure
  HiLink kasmTypeError		Error

  " Directive Group:
  HiLink kasmConstant		Constant
  HiLink kasmInstrModifier	Operator
  HiLink kasmRepeat		Repeat
  HiLink kasmDirective		Keyword
  HiLink kasmStdDirective	Operator
  HiLink kasmFmtDirective	Keyword

  " Register Group:
  HiLink kasmCtrlRegister	Special
  HiLink kasmDebugRegister	Debug
  HiLink kasmTestRegister	Special
  HiLink kasmRegisterError	Error
  HiLink kasmMemRefError	Error

  " Instruction Group:
  HiLink kasmStdInstruction	Statement
  HiLink kasmSysInstruction	Statement
  HiLink kasmDbgInstruction	Debug
  HiLink kasmFpuInstruction	Statement
  HiLink kasmMmxInstruction	Statement
  HiLink kasmSseInstruction	Statement
  HiLink kasmNowInstruction	Statement
  HiLink kasmAmdInstruction	Special
  HiLink kasmCrxInstruction	Special
  HiLink kasmUndInstruction	Todo
  HiLink kasmInstructnError	Error

  delcommand HiLink
endif

let b:current_syntax = "kasm"

" vim:ts=8 sw=4
