
: OBJECT.CLS.VER
    "OBJECT.CLS V1.1 4/1/90 JW"
;

\ object.cls.ver type cr

\   Temporary definition of object compiling words

variable object.self                  \ Instance current method is referencing
variable object.classvar              \ Current class variable buffer
variable object.head                  \ Class definition head
variable object.inum                  \ Number of variables in class definition
variable object.off                   \ Instance storage offset
variable object.coff                  \ Class variable storage offset
variable object.mode                  \ Current variable mode
variable object.flags                 \ Current variable flags
variable object.patch1                \ Backpatch length
variable object.classmodes            \ Class definition modes
variable object.methtype              \ Method type bits
variable object.methargs              \ Processing arguments of method
variable object.inargs                \ In static argument list
variable object.methdef               \ Argument default value
variable object.methbase              \ Argument base point
variable object.methmodes             \ Argument acquisition modes
variable object.methkw                \ Argument keyword list

\   Extended entity data result item types

 0 constant E_string
 5 constant E_pointer
10 constant E_triple
11 constant E_position
12 constant E_displacement
13 constant E_direction
40 constant E_real
41 constant E_distance
42 constant E_scalefactor
71 constant E_integer
99 constant E_instance

\   Pseudo data types for variant argument forms

140 constant E_angle
240 constant E_orientation
111 constant E_corner
100 constant E_keyword

\   Storage classes

0 constant S_static
1 constant S_private
2 constant S_protected
4 constant S_public
8 constant S_temp

\   Storage flags

0  constant F_instance
16 constant F_class

\   Class mode bits

1 constant  C_dpublic                 \ Public derived class

\   Method type bits

0  constant M_atlas                   \ Normal method, accessible from Atlas
16 constant M_command                 \ Command method, callable from AutoCAD
32 constant M_external                \ External method, in another application

\   Drawing modes for the turtle

0  constant D_nothing                 \ Just screen vectors
1  constant D_polylines               \ 3D Polylines for connected vectors
2  constant D_faces                   \ Rat nest mesh polygon faces
3  constant D_lines                   \ Unconnected Lines

\   Argument acquisition modes

1  constant ARG_nonull                \ No null input
2  constant ARG_nozero                \ No zero input
4  constant ARG_noneg                 \ No negative input
8  constant ARG_nolim                 \ Don't check limits
16 constant ARG_3d                    \ Return 3D points       (OBSOLETE??)
32 constant ARG_dashed                \ Use dashed rubberband lines
0x1000 constant ARG_cronly            \ CR delimited input string

\   General purpose constants

-1 constant True
0  constant False
1.0 atan 4.0 f* 2constant Pi

(   CLASSOK  --  Verify that the address on the top of the stack is
                 a valid class definition.  )

: CLASSOK                             ( classhdr -- classhdr )
    dup @ 0x13508F1D <> if abort" "Not a class!\n" then
;

\   (ARGLE)  --  Argument acquisition reset

: (ARGLE)
    0 object.methdef !                \ No default
    0 object.methbase !               \ No base point
    0 object.methmodes !              \ Default acquisition modes
    0 object.methkw !                 \ No keyword list
;

(   CLASS  --  Begin a class definition.

    This creates the following identifier as a class name, compiles
    the class sentinel in the class header structure, and reserves
    space for the class properties which will be backpatched when
    the ENDCLASS is encountered based on the variables seen within
    the class.

    BEWARE!!  If you change the structure compiled by CLASS, you
    must make corresponding changes to the CLASSDECL structure
    in ADSCLASS.C and the code the processes it.
)

: CLASS
    1 (redefine)                      \ Allow temporary redefinitions
    here object.head !                \ Definition head address
    create                            \ Create the variable
    0x13508F1D ,                      \ Class sentinel
    here object.patch1 !              \ Save backpatch address
    0 ,                               \ Backpatch with instance variable count
    0 ,                               \ Backpatch with instance length
    0 ,                               \ Patched with classitem pointer
    0 ,                               \ Patched with class variable length
    0 ,                               \ Parent class
    0 ,                               \ Class mode bits
    0 object.classmodes !             \ Reset class modes
    0 object.inum !                   \ Number of class variables
    0 object.off !                    \ Offset within instance
    0 object.coff !                   \ Offset within class
    M_atlas object.methtype !         \ Methods default to Atlas-only
    F_instance object.flags !         \ Clear storage flags
    S_private object.mode !           \ Default mode is private
    0 object.methargs !               \ Set to declare variables, not args
    0 object.inargs !                 \ Reset argument mode to static
    (argle)                           \ Reset argument acquisition
;

(   INSTANCE  --  Declare an instance of a class

    If you've declared a class named LEISURE, you can declare an
    instance of it namd VACATION with:

        :leisure INSTANCE vacation

    This reserves storage for an instance of the named class.
)

12 constant Civl                      \ Class instance variable header

: INSTANCE                            ( classhdr -- )
    classok                           \ Validate that argument is a class
    create                            \ Create the class instance
    1 object.inum +!                  \ Increment variable count
    object.mode @ S_static = if
        \ Compile static instance outside a class definition
        here over 8 + @ Civl +        \ Instance variable length in bytes
        allot c= (construct)          \ Allocate and construct static instance
        does>
            Civl +                    \ Return pointer to instance area
    else
        \ Compile an instance within a class definition

        dup 8 + @ Civl +              \ Get instance variable length
        3 + 4 / 4 *                   \ Round storage size to even stackitem
                              ( Emplace mode word )
\       |        length:16          |   type:8   | flags+mode:8 |
        E_instance 8 shift over 16 shift or object.flags @ or
            object.mode @ or dup ,
        0 object.flags !              \ Reset flags to default (instance var)
        dup S_temp and 0= if
            F_class and 0= if
                ( Allocate storage for variable in the instance if it isn't a
                  class variable and it's not a temporary used in the method
                  code. )
                object.off @ ,        \ Compile current object offset
                object.off +!         \ Update instance offset
\ ." "Nested instance class: " dup . cr
                ,                     \ Compile class definition pointer
                does>
                    4 + @ Civl +      \ Load offset within instance
                    object.self @ +   \ Base on active instance pointer
                    exit
            else
                ( Allocate storage for variable in the class if it's a
                  class variable. )
                object.coff @ ,       \ Compile current class offset
                object.coff +!        \ Update class offset
                ,                     \ Compile class definition pointer
                does>
                    4 + @ Civl +       \ Load offset within instance
                    object.classvar @ + \ Base on active instance pointer
                    exit
            then
        else
            ( This is a temporary variable.  Allocate
              static heap storage for it and define its action as
              returning a pointer to the storage area. )
\  ." "Allot temp instance: " dup . cr
            drop                      \ Discard extra copy of flags
            here swap allot           \ Allocate space on the heap
            (construct)               \ Run the constructor to initialise it
            does>
                [ 4 Civl + ] literal + \ Skipping the mode, of course
                exit
        then
    then
;

(   CLASS.INSTANCE.DECL  --  Create instance declaring word

    Rather than using INSTANCE directly, most instances are declared
    with the declarator given the same name as the class.  For
    example, to declare an instance of the class LEISURE, just use:

        LEISURE suit

    At the time CLASS.INSTANCE.DECL is called, the declaring word has
    just been defined and listed on the dictionary, but no code has been
    compiled for it.  This word compiles in the actual code for the
    word, having been passed the class declaration item on the stack.
)

: CLASS.INSTANCE.DECL
    ['] (lit) , ,                     \ Push inline class defintion
    ['] instance ,                    \ Declare an instance of that class
    ['] exit ,                        \ That's all, folks
;

(   'S-CLASS  --  Obtain class an instance belongs to.  )

: 'S-CLASS                            ( instance -- classhdr )
    Civl - @                          \ Back up to header, point to it
;

(  CLASSNAME  --  Store name of class in string.  )

: CLASSNAME                           ( classhdr string --  )
    swap body> >name swap name>s!
;

(   CLASS.DEFVAR  --  This is called by all the data type declaration
                      words to actually declare the variable.  When
                      called, it expects the variable type [DXF code]
                      and the number of bytes to be reserved for the
                      variable on the stack.  In addition, the
                      variable OBJECT.FLAGS will have been preset to
                      F_class if a STATIC modifier preceded the declaration,
                      and OBJECT.MODE will contain the current mode for
                      variable declarations [Public, Private, or Temporary].

                      Depending upon these inputs, we define the variable
                      in one of several different ways.  If the variable
                      is a STATIC class variable, we actually reserve
                      storage for it in line and declare the variable
                      conventionally as pushing its address on the stack.

                      If the variable is an instance variable, we treat it
                      very differently.  We define a word, using DOES>, that
                      pushes the sum of the contents of the global variable
                      SELF and the current instance variable offset onto
                      the stack.  We then add the length of this variable
                      to the current offset.  When the variable is used
                      subsequently, it will generate an indirect reference
                      to the currently active class instance.

                      We can also use an instance variable to access a
                      member of a class other than our own by using
                      the FIELD operator.  Given the address of a class
                      instance, we can say:

                              furbish lousewort.size FIELD

                      to obtain the address of that field within the
                      instance of the LOUSEWORT class named FURBISH.
)

: class.defvar                        ( Type nbytes --  )
    object.methargs @ if              \ If we're declaring arguments
        drop                          \ Ditch the size
        exit                          \ And just leave the type on the stack
    else
        swap 100 mod swap             \ Get primitive type of variable
    then
    3 + 4 / 4 *                       \ Round storage size to even stackitem
    create                            \ Create variable
    1 object.inum +!                  \ Increment variable count
    object.mode @ S_static = if       \ If declaration is outside a class
        4 / 0 ?do 0 , loop            \ Just allocate its storage
        drop                          \ Ditch its field type
    else
                                      \ Emplace mode word
\       |        length:16          |   type:8   | flags+mode:8 |
        swap 8 shift over 16 shift or object.flags @ or
            object.mode @ or dup ,
        0 object.flags !              \ Reset flags to default (instance var)
        dup S_temp and 0= if
            F_class and 0= if
                ( Allocate storage for variable in the instance if it isn't a
                  class variable and it's not a temporary used in the method
                  code. )
                object.off @ ,            \ Compile current object offset
                object.off +!             \ Update instance offset
                does>
                    4 + @                 \ Load offset within instance
                    object.self @ +       \ Base on active instance pointer
                    exit
            else
                ( Allocate storage for variable in the class if it's a
                  class variable. )
                object.coff @ ,           \ Compile current class offset
                object.coff +!            \ Update class offset
                does>
                    4 + @                 \ Load offset within instance
                    object.classvar @ +   \ Base on active instance pointer
                    exit
            then
        else
            drop                          \ Ditch flags (irrelevant if temp)
            ( This is a temporary variable.  Allocate
              static heap storage for it and define its action as
              returning a pointer to the storage area. )
\           ." "Allot temporary variable: " dup . cr
            4 / 0 ?do 0 , loop        \ Allocate static storage
            does>
                4 +                   \ Skipping the mode word, of course
                exit
        then
    then
;

(   Primitive object type declarators  )

: INTEGER                             ( Untransformed 32 bit integer )
    E_integer 4 class.defvar
;

: REAL                                ( Untransformed real number )
    E_real 8 class.defvar
;

: SCALEFACTOR                         ( Scale factor )
    E_scalefactor 8 class.defvar
;

: DISTANCE                            ( Distance scalar )
    E_distance 8 class.defvar
;

: POINT                               ( Position in space )
    E_position 24 class.defvar
;

: TRIPLE                              ( Nontransformed triple of numbers )
    E_triple 24 class.defvar
;

: DISPLACEMENT                        ( Displacement vector )
    E_displacement 24 class.defvar
;

: DIRECTION                           ( Orientation vector )
    E_direction 24 class.defvar
;

: CHARACTERS                          ( Character string )
    E_string object.methargs @ 0= if swap else 0 then class.defvar
;

: POINTER                             ( Database handle )
    E_pointer 20 class.defvar
;

(   Pseudo data types used for acquisition  )

: ANGLE                               ( Planar relative angle )
    E_angle 8 class.defvar
;

: ORIENTATION                         ( Planar bearing angle )
    E_orientation 8 class.defvar
;

: CORNER                              ( Corner of box )
    E_corner 24 class.defvar
;

: KEYWORD
    E_keyword object.methargs @ 0= if swap else 0 then class.defvar
;

(   Variable mode declarators  )

: PRIVATE:
    S_private object.mode !
;

: PROTECTED:
    S_protected object.mode !
;

: PUBLIC:
    S_public object.mode !
;

(   Storage class declarators  )

: TEMPORARY:
    S_temp object.mode !
;

: STATIC
    F_class object.flags !
;

(   PUBLIC  --  Used in a :classname PUBLIC DERIVED construction to
                denote external inheritance of all the methods of
                the parent class.  )

: PUBLIC
    C_dpublic object.classmodes !
;

: DERIVED
    classok object.head @ swap object.classmodes @ (inherit)
    ( Initialise the variable declaration counters to the
      values from the parent class returned on the stack
      by [inherit]. )
\ ." "Inherit " .s cr
    object.inum ! object.off ! object.coff !
;

(   CLASS.ERR  --  Reset state if class load errors.  )

: CLASS.ERR
    0 object.head !                   \ Mark no class open
    S_static object.mode !            \ Mark declarations outside class static
;

(   CLASS.END  --  End class definition and postprocess.  )

: CLASS.END
    0 (redefine)                      \ Resume warnings about redefinitions
    object.inum @ object.patch1 @ !   \ Backpatch header with variable count
    object.off @ object.patch1 @ 4 + ! \ Backpatch header with instance length
    object.coff @ object.patch1 @ 12 + ! \ Backpatch header with class length
\   ." "Variables: " object.inum @ . ." "Instance length: " object.off @ . cr
\   "Hey!! We got to the end of the class.\n" type
    object.head @ (endclass)          \ Postprocess the class definition
    class.err                         \ Reset state to no class active
;

(   METHOD.PROLOGUE  --  Common initialisation code run upon entry
                         to every method.  )

: METHOD.PROLOGUE
\ ." "Entering method\n"
    r@                                \ Get in-line temp address off return stk
    dup object.self @ swap !          \ Save caller's self in first temp
    4 + dup object.classvar @ swap !  \ Save caller's classvars in second temp
    4 +                               \ Get actual in-line return address
    >r                                \ Place that on return stack
    object.focus @ ?dup drop          \ If focused on a given object, use it
    dup object.self !                 \ Set current object
    's-class (classvars) object.classvar ! \ Set class variables
    1 object.methargs !               \ Allow arguments to be obtained
;

(   METHOD.EPILOGUE  --  Common clean-up code run when leaving every
                         method.  )

: METHOD.EPILOGUE
\ ." "Leaving method.\n"
    r>                                \ Get our return address
    r>                                \ Get method's context save area
    dup @ object.self !               \ Restore caller's self pointer...
    4 + @ object.classvar !           \ ...and class variable pointer
    >r                                \ Replace our return address on rtn stack
    0 object.methargs !               \ Disable argument acquisition
;

(   COMMAND  --  Mark the next method to be defined as an AutoCAD
                 command.  )

: COMMAND
    M_command object.methtype @ or object.methtype !
;

(   EXTERNAL  --  Mark the next method as defined in an external application
                  to be invoked for this method.  )

: EXTERNAL
    M_external object.methtype @ or object.methtype !
;

(   METHOD  --  Begin compilation of a method.  )

: METHOD
    -1 object.methargs !              \ Allow argument declarations
    0 object.inargs !                 \ But not in static list yet
    (argwipe)                         \ Clear argument processing queue
    create                            \ Define next word
;

\   ((  --  Begin method argument declaration.

: ((
\   -1 object.methargs !              \ Mark processing arguments
    -1 object.inargs !                \ In static argument list
    (argle)                           \ Reset for first argument
;

\   ))  --  End method argument declaration.

: ))
\   0 object.methargs !               \ Done processing arguments
    0 object.inargs !                 \ Outside the static argument list
; immediate

(   ARG  --  Declare an argument.  )

: ARG
(   ** DEBUG **
    ." "Argument type: " over . ." " Prompt: `" dup type ." "'\n"
    object.methdef @ ?dup if
        ." "  Default at: " . cr
    then
    object.methbase @ ?dup if
        ." "  Base point at: " . cr
    then
    object.methmodes @ ?dup if
        ." "  Acquisition modes: " . cr
    then
    object.methkw @ ?dup if
        ." "  Keyword list: " type cr
    then
)
    object.methdef @ object.methbase @ object.methmodes @ object.methkw @
    (argdef)                          \ Tack argument onto argument queue
    (argle)                           \ Reset modes for next argument
    object.inargs @ 0= if             \ If outside static argument list
        (arg_get)                     \ Acquire the argument immediately
    then
;

\   (POINTEROK?)  --  Check if pointer on stack is valid.

: (POINTEROK?)                        ( pointer -- pointer )
    dup @ drop                        \ Let Atlas check the pointer
;

(   DEFAULT  --  Specify pointer to argument's default value.  )

: DEFAULT
    (pointerok?)
    object.methdef !                  \ Set default pointer
;

(   BASEPOINT  --  Specify pointer to base point for acquisition.  )

: BASEPOINT
    (pointerok?)
    object.methbase !                 \ Set base point pointer
;

(  ARGMODES  --  Specify argument acquisition modes [ads_initget bits].  )

: ARGMODES
    object.methmodes !                \ Save acquisition modes
;

(   KEYWORDS  --  Specify keywords recognised at argument prompt.  )

: KEYWORDS
    (pointerok?)
    object.methkw !                   \ Save pointer to string
;

(   {  --  Prefix code segment of method.  This actual reason for the
           { word is to allow us to rename the just-defined method into
           its hidden internal name and to create a virtual function
           table item providing access to this method for objects of
           this class.
)

: {
    object.head @ object.mode @ object.methtype @ or (methodefine)
    ['] method.prologue ,             \ Compile prologue
    0 , 0 ,                           \ Reserve space to save context
    -1 state !                        \ Begin compilation
; immediate

(   }  --  End compilation of a method.  )

: }
    ['] method.epilogue ,
    ['] ; execute
    object.methtype @ object.mode @ 5 shift or
    (wordbits@) or (wordbits!)        \ Set modes in word
    M_atlas object.methtype !         \ Reset method type to default
    0 object.methargs !               \ Disable argument processing
; immediate

(   RETURN  --  Return from a method.  We must use RETURN rather than
                EXIT so that the epilog gets executed to restore the
                caller's context and adjust the return stack. )

: RETURN
    ['] method.epilogue ,             \ Compile call on epilogue...
    ['] exit ,                        \ ...and then an exit from the word
; immediate

\   Load co-ordinate triple onto stack

: POINT@
    dup >r 2@ r@ 8 + 2@ r> 16 + 2@
;

\   Store co-ordinates on stack into point variable

: POINT!
    dup >r 16 + 2! r@ 8 + 2! r> 2!
;

\   Print point co-ordinates

: POINT?
    dup >r 2@ f. r@ 8 + 2@ f. r> 16 + 2@ f.
;

(   CLASS.STARTDRAW  --  Initialise to draw new geometry.  )

: CLASS.STARTDRAW
    reset 1 leavetracks               \ Put turtle at 0,0; pen down
;

(   CLASS.ENDDRAW  --  Close out after creating geometry.  )

: CLASS.ENDDRAW
    reset
;

(   SCRATCHPAD  --  Given the name of a class, obtain the scratchpad
                    instance it uses for I/O.  )

: SCRATCHPAD                          ( :classname -- instance )
    classok
    (instvars)
;

(   OBJ!  --  Copy instance variables from one object to another.
              The objects must be of the same type.  )

: OBJ!                                ( source dest obj! -- )
    2dup 's-class swap 's-class <> if
        abort" "Objects are not of the same class.\n"
    then
    dup 's-class 8 + @                \ Get instance variable storage length
\ ." "Copying " dup . ." "bytes of instance variables.\n"
    -rot Civl - swap Civl - swap rot
    Civl +                            \ Copy header as well
    4 / 0 ?do
        over @ over !
        4 + swap 4 + swap
    loop
    2drop
;

(   FOCUS  --  Set focus on specified object.  )

: FOCUS                               ( instance --  )
    object.focus !
;

(   DEFOCUS  --  Remove focus.  Methods return to requiring an explicit
                 instance on the top of the stack.  )

: DEFOCUS
    0 focus
;

(   THIS  --  Return the current object instance within a method.  )

: THIS
    object.self @
;

(   <-  --  Reference virtual method or parent class message.  )

: <-
    object.head @ (<-)
; immediate
