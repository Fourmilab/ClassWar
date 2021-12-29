# ClassWar
## Class Language Application Support System Within AutoCAD. Really!

Software development, like business administration, is prone to fads.
It seems like every five or ten years some new craze captures the
imagination of those in the industry (or, more commonly, and worse,
their managers, who only dimly understand the details of the matter
but have heard the buzzwords on the lips of their peers).  In the
latter part of the 1980s and early 1990s, one consuming passion was
“object orientation”, which has its roots in languages such as Simula
from the 1960s and Smalltalk in the 1970s, and became all the rage
when languages such as C++ and Objective C brought some its concepts
into the mainstream.

The computer-aided design (CAD) world was, arguably, one of the first
to adopt an object model, with Ivan Sutherland's Sketchpad system,
developed in 1960 and 1961, embodying concepts such as definitions,
objects, and instances.  In the late 1980s, workstation CAD vendor
Intergraph jumped on the object oriented bandwagon by promoting their
system as having an “object oriented database”.  It was, of course,
nothing of the sort, being simply a conventional geometry database
with the latest buzzword glued on by the marketing people.  It was
just like the chrome plating I had mocked in the era by declaring my
software “Turbo Digital”, embodying two of the hottest 1980s consumer
product Pavlov stimuli, and proclaiming “Digital isn't enough any
more”.

Object orientation, as usually defined, is built upon four pillars:

* Abstraction
* Encapsulation
* Inheritance
* Polymorphism

which are, in any case, more properties of a programming language than
a database.

In 1990, Autodesk was working toward the release of AutoCAD Release 11,
one of whose major features was the AutoCAD Development System (ADS),
which allowed users and third-party developers to extend AutoCAD with
C language programs that ran as fast as AutoCAD's internal code, adding
new objects, commands, and database information without requiring any
changes to AutoCAD.  Concurrently, in early 1990 I had completed
development of [ATLAST](https://www.fourmilab.ch/atlast/), the
“Autodesk Threaded Language Application System Toolkit”, a FORTH-like
embedded language that was originally intended (but never actually
used) to communicate between a “headless” AutoCAD and machine-specific
front-ends customised for the window systems and user interface
paradigms of platforms that ran AutoCAD.

It occurred to me that by using ADS and ATLAST as the foundation, I
could, with a modest amount of effort, create a *genuine* object
oriented CAD system, provide a worked example of an ambitious ADS
application for developers wishing to learn the system, and demonstrate
how ATLAST made building fast, open, and extensible systems easy.  I
could also have some fun at the expense of Intergraph, which I was
[always up for](https://www.fourmilab.ch/autofile/e5/chapter2_46.html#n2639).

Thus was born ClassWar, a rather pained acronym for “Class Language
Application Support System Within AutoCAD. Really!” which additionally
tweaked Intergraph, whose sales force was deriding AutoCAD as “toy
CAD for unserious people”, with the typical disdain of established
aristocracy for proletarian upstarts.  ClassWar turned AutoCAD database
objects (entities) into programmable objects, which could embed ATLAST
code and ClassWar extensions that implemented every one of the
pillars of object orientation.  It allowed users and developers to
program new objects with their own behaviour, and to build upon
libraries of previously-developed objects when developing their own.

ClassWar shipped with AutoCAD Release 11 as one of a number of ADS
demonstration programs but was never an officially-supported Autodesk
product.  Several customers experimented with it and built custom
objects using it but, to my knowledge, none of these were brought to
market or widely used.

ClassWar is almost entirely forgotten today, but not be me, who had a
lot of fun designing, implementing, and demonstrating it at the time,
and making fun of Intergraph's object oriented pretensions.  I doubt if
anybody could get the code to run forty years later, even if there were
any conceivable reason for doing so.  Consequently, this is a static
archive of the code as it was in May 1990, and will not change. (Some
files have modification dates which are more recent, but these are due
to formatting changes which do not affect the code \[for example,
replacing hardware tabs with spaces, eliding trailing spaces, and
making end of line characters consistent\].)

---
John Walker\
May, 1990\
19,459 lines of code

