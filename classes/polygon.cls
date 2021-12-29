
\   Polygon class definition

    ( The PRIVATE: word introduces declarations of class instance
      variables accessible only from within the class itself.  These
      variables are saved in instances of the class as entity attributes
      of the corresponding type. )

    ( The PROTECTED: word introduces declarations available to classes
      derived from this class, but not to other classes which operate
      upon instances of this class. )

PROTECTED:

    ( The STATIC word introduces declarations of variables shared
      by all members of the class.  The values of these variables
      are saved in the drawing as entity attributes attached to
      the class definition block itself.  The variables are named
      <class.varname>. )

    STATIC INTEGER polycount          \ Total polygons in drawing

    REAL size                         \ Radius of circumscribed circle
    INTEGER nsides                    \ Number of sides

    ( The PUBLIC: word introduces declarations of class instance
      variables accessible to other Atlas code.  Given the variable
      ID and an entity, anybody can change them. )

PUBLIC:
    INTEGER polyseq                   \ Unique sequence number

    ( The TEMPORARY: word introduces declarations of transient
      variables used only within class-implementing code.  These
      variables behave like STATIC: variables, but are not saved
      in the class definition entity and hence are lost at the end
      of an editing session.  They should not be assumed to retain
      data from one word invocation to another. )

TEMPORARY:
    REAL anginc                       \ Angle increment

: deg_rad
    Pi f* 180.0 f/
;

PRIVATE:

method newclass
{
    2000 polycount !                  \ Initialise polygon sequence numbers
}

PUBLIC:

    ( The DRAW word is one of the most fundamental in any class
      definition.  When invoked with the class variables containing
      the values for an instance, it must create the entity or
      entities that represent the geometry for that object.  The
      values of the class variables will then be automatically affixed
      to the entity it creates.  )

method draw
{
    360.0 nsides @ float f/ 2dup anginc 2!
    penup
        nsides @ 1 and 0= if
            2dup 2.0 f/ 90.0 f+ 2dup right size 2@ forward left
        else
            90.0 2dup right size 2@ forward anginc 2@ 2.0 f/ f+ left
        then
    pendown
    deg_rad cos 1.0 f- fnegate size 2@ 2dup f* 2.0 f* f* sqrt
    nsides @ 0 do
        2dup forward anginc 2@ left
    loop
    2drop
}

    ( The presence of an ACQUIRE word declares a constructor for
      instances.  This will cause an AutoCAD command with the same
      name as the class to be defined.  When the command is entered,
      the ACQUIRE word is run to obtain the instance variable settings
      from the user, and then the DRAW method is invoked [if defined]
      to create the geometry for the instance.  )

method acquire
{
    "Number of sides: " getint 0= if drop False return then nsides !
    "Edge size: " getreal 0= if 2drop False return then size 2!
    1 polycount +! polycount @ polyseq !
    True
}

    ( The GROW method is defined on polygon objects.  Since methods are
      polymorphic, any number of classes can contain GROW methods.  The
      type of object picked determines which is invoked. )

command method grow (( real "Growth factor" ARG_nozero ARG_noneg or argmodes
                        arg ))
{
    size 2@ f* size 2!
}

variable ione 1 ione !

    ( MORE  --  Increase number of sides. )

command method more (( integer "Additional sides" ione default arg ))
{
    nsides +!
}

    ( LESS  --  Decrease number of sides. )

command method less (( integer "Fewer sides" ione default arg ))
{
    negate nsides +!
}
