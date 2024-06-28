## About GPE ##

Started in early 2002 GPE was one of the early Open Source projects that 
intended to make Linux-based mobile devices useful for everyone. At that time 
iPaq, SimPad and other devices developers ported Linux to were the target 
platforms for GPE. Initially the project was hosted at handhelds.org and using 
Subversion for revision control. This platform is history as well as 
linuxtogo.org which was used later.

GPE described itself like this on its project website:
The GPE Palmtop Environment (GPE) is a collection of integrated software 
components optimized for (but not limited to) handheld and other input 
constrained and resource limited devices. GPE provides PIM (calendaring, todo 
management, contact management and note taking), Multimedia (audio playback and
image viewing) and connectivity solutions (web browsing). Another major goal of 
GPE is to encourage people to work on free software for mobile devices and to 
experiment with new technologies.

GPE provides an infrastructure for easy and powerful application development by 
building on available technology including GTK+, SQLite, DBus and GStreamer and 
several more common standards defined by freedesktop.org.

GPE is committed to the Open Source idea. All GPE core components are released 
under GNU licenses, applications using the GPL and shared libraries using the 
LGPL. Those allow for the most free usability of the GPE system. 


## About GPE Git ##

The GPE Palmtop Environment Git source code repository was created from the 
original Subversion (svn) repository. This originates from the latest 
linuxtogo.org backup:

```
UUID: d4735bf7-8e1f-0410-bce4-ef584eb11c25
Revision: 10145
Last Changed Author: fboor
Last Changed Rev: 10145
Last Changed Date: 2013-04-18 11:01:44 +0200 (Thu, 18 Apr 2013)
```

I've used a local SVN server and the following command to convert the repository:

```
svn2git --authors authors.txt svn://127.0.0.1/
```

The used authors mapping file *authors.txt* is part of the Git tree.

The main motivation is to keep the historic sources publicly available and 
maybe even useful for someone.

Florian Boor 2024/06/27
