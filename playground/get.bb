// main.bb
// grant necessary read access rights
!access "https://" 
!access "ftp://" 
start = time();

// join the string contents of the file iterable
google = "https://www.google.com"|file|bb.string.join("\n");

// download some data from a content
gnu = "ftp://ftp.gnu.org/README"|file;
//result["username"] = "demo";
//result["password"] = "password";
gnu |= bb.string.join("\n");

print("Google response length: {google|len}");
print("Rebex response length: {gnu|len}");
print("Response time: {time()-start} sec");