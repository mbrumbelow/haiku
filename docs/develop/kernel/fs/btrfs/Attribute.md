# Attribute
Class used to represent file attributes.
# Public members
## Attribute(Inode\* inode);
Constructs an Attribute object for the nameless file corresponding to the Inode 
object pointed by inode.
### Parameters
#### \*inode
Pointer to an Inode object for which the Attribute object is to be constructed.
## Attribute(Inode\* inode, attr_cookie\* cookie);
Constructs an Attribute object for the file corresponding to the Inode object
pointed by inode and whose name is in the object pointed by cookie.
### Parameters
#### \*inode
Pointer to an Inode object for which the Attribute object is to be constructed.
#### \*cookie
Cookie containing the name of the file for which the Attribute object is to be
constructed.
## \~Attribute();
Object destructor
## status_t            CheckAccess(const char\* name, int openMode);
Checks access for the file named \*name with flags set to openMode
## status_t            Create(const char\* name, type_code type, int openMode, attr_cookie\*\* \_cookie);
## status_t            Open(const char\* name, int openMode, attr_cookie\*\* \_cookie);
Opens the file named \*name with flags openMode and attr_cookie initialized to
value of pointer \_cookie
\*\*\_cookie
## status_t            Stat(struct stat& stat);
Reads the attributes of the this object into stat
## status_t            Read(attr_cookie\* cookie, off_t pos, uint8\* buffer, size_t\* \_length);
Reads the attributes of the this object into \*buffer.
