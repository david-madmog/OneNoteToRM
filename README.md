
# OneNote to RM

OneNote to RM is a MS Windows application that synchronises ReMarkable documents to and from Microsoft OneNote.

It uses the ReMarkable cloud api to access  ReMarkable documents and the [Microsoft Graph API](https://learn.microsoft.com/en-us/graph/overview) to manage OneNote douments in OneDrive.

It is intended to allow documents to be edited on either the ReMarkable device or in OneNote and then the document can be sent to the other
environment for further editing. Note, unlike the native RM integrations, it actually translates the native lines formats rather than converting to
PDF, so allows unlimited editing and re-editing in either environment.

The focus is on translating the actual lines, though it will translate text also. Documents are translated in a format that looks similar. However, due to different underlying archetectures and approaches, not all information is preserved in a round trip; for example pen styles can be lost.

**This is still a work in progress and subject to change. Do not use this for important information - take a local copy first!**

## Installation

There's an msi package included in the release, and I've included the source from the wix installer as a part of the visual studio solution.

## Usage

The application runs in interactive mode on a Windows desktop or from a windows command line.

### Setup 

On the first run, the application will need to be configured for permissions to access both the RM Cloud and the MS Graph API. You may need to refresh the document lists after logging in.

#### Log in to Microsoft
The application will present a Microsoft login dialog and ask for permissions to access the OneNote files. Once this is granted, it will obtain a token that is valid for 90 days, and is refreshed on each run. Once the token is obtained it should not need to ask again, unless the app is not used for a period of time, in which case it will need re-authenticating (but should not ask for permissions to be granted again).

#### Log in to RM Cloud
A dialog will be presented. Go to the ["My ReMarkable" page](https://my.remarkable.com/#desktop) to obtain an 8-character key associated with your account. Enter this key in the dialog and the app will obtain and cache a key. 

### Interactive mode

Once the documents are listed, there are three main features:

- Preview: will pop up a window showing a preview of the highlighted RM or OneNote document

- Overwrite → and ← : Will copy the highlighted source document to the highlighted destination document (irrevocably trashing the existing contents of the destination)

- ← ? → : Will copy the highlighted source document to the destination document, based on whichever is updated most recently. 
		If a destination document of that name already exists, it will be irrevocably overwritten. 

- ← &#128337; → : Will scan every minute, and if either document has changed since the last scan, will copy the newer one to the older one.
		If both have changed since the last update, changes to the older one will be irrevocably trashed.

### Command-line mode

Command-line mode will only work if logon tokens have previously been obtained in interactive mode. It is intended to be used in scheduled tasks or batch files. 

Command line switches are as follows:

```
OneNoteToRM.exe -I <Input Document> -O <Output Document> -M<Mode> -L <time>
```

- -MR: Remarkable to OneNote
	- -I is RM document name
	- -O is mandatory as name of Notebook to insert section into
- -MO: OneNote to Remarkable
	- -I use format Notebook/Section
	- -O is ignored
- -MT: Timed mode (Will do nothing if neither is updated since last T mode invoked)
- -ML: Timed/Loop mode 
   	- -I - OneNote name: use format Notebook/Section
    - -O - RM document Name
- L: Loop mode; will perform the action, wait for specified number (in seconds) and repeat indefinitely

## Architecture and sharing

The body of the work is now implemented as a Windows DLL (implemented in C++) and the UI is C#, calling the DLL for all work. 

The DLL features an API allowing you to create both RM and OneNote documents (in internal format) for display or conversion, load and save documents, and render/convert to either a document of the other type or a Windows Device Context (DC) for display.

If you are interested in using the DLL for your own project, you are very welcome (subject to the terms of the license). I am happy to help, please do contact me.

## Acknowledgements and references

### RMDoc

The format of ReMarkable .rmdoc files is not officially documented. I made heavy use of the knowldge uncovered by the following:

- Rick Lupton's RMScene - <https://github.com/ricklupton/rmscene/tree/main>
- ddvk's work - <https://github.com/ddvk>
- Excustic's further development of RMC, including the InkML exporter - <https://github.com/Excustic/rmc>

### RM Cloud API

Initial versions shelled out to the excellent [RMAPI](https://github.com/ddvk/rmapi) application originally developed by Javier Uruen Val (https://github.com/juruen) and forked by ddvk.
In the end, I decided it was better to use the RM cloud API directly, so I reverse-engineered RMAPI for the latest API abd with the help of Fiddler and some guesswork, managed to get it going.
Note, there's a lot of documentation on the older version of the API, but very little on the newer one!

### OneNote (Graph) API

The native OneNote document format is so complex as to be unusable. However, apart from the complexity of setting up the OAuth 
for logon, the Graph API is relatively straightforward.
	
- see <https://learn.microsoft.com/en-us/graph/integrate-with-onenote> for API description.
- see <https://microsoft.github.io/cpprestsdk/classweb_1_1http_1_1http__request.html> for documentation on C++ REST API

### Utility libraries
I also used the following utility libraries:
- nlohmann/json parser for building the body of requests and managing the responses. See <https://json.nlohmann.me/api/basic_json/> for JSON docs
- pugixml C++ XML processing library for processing the HTML and InkML documents: <https://pugixml.org>
- SHA256 algorithm was from <https://github.com/kibonga/sha256-cpp> (MIT License applies - not included in this repo)
- Base64 encoding was from <https://github.com/tobiaslocker/base64> ((MIT License applies)
