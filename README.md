
# OneNote to RM

OneNote to RM is a MS Windows application that synchronises ReMarkable documents to and from Microsoft OneNote.

It uses the ReMarkable cloud api (through use of the excellent [RMAPI](https://github.com/ddvk/rmapi) application) to access  ReMarkable 
documents and the [Microsoft Graph API](https://learn.microsoft.com/en-us/graph/overview) to manage OneNote douments in OneDrive.

It is intended to allow documents to be edited on either the ReMarkable device or in OneNote and then the document can be sent to the other
environment for further editing. Note, unlike the native RM integrations, it actually translates the native lines formats rather than converting to
PDF, so allows unlimited editing and re-editing in either environment.

The focus is on translating the actual lines, though it will translate text also. Documents are translated in a format that looks similar. However, due to different underlying archetectures and approaches, not all information is preserved in a round trip; for example pen styles can be lost.

**This is still a work in progress and subject to change. Do not use this for important information - take a local copy first!**

## Installation

*TO DO.* There's no installation yet. For now, you'll have to download the source and build it yourself. You'll also have to install RMAPI directly from [DDVK's Github](https://github.com/ddvk/rmapi) 

Sorry.

## Usage

The same application runs in either interactive or command-line mode

### Interactive mode

On the first run, the application will need to be configured for permissions to access both the RM Cloud and the MS Graph API.

*TO DO: Document logging in with RMAPI*

The application will present a Microsoft login dialog and ask for permissions to access the OneNote files. Once this is granted, it will obtain a token that is valid for 90 days, and is refreshed on each run. Once the token is obtained it should not need to ask again, unless the app is not used for a period of time, in which case it will need re-authenticating (but should not ask for permissions to be granted again).

Once the documents are listed, there are three main features:

- Preview: will pop up a window showing a preview of the highlighted RM or OneNote document

- Overwrite → and ← : Will copy the highlighted source document to the highlighted destination document (irrevocably trashing the existing contents of the destination)

- Use Name → and ← : Will copy the highlighted source document to a destination document of the same name as the source (for OneNote, in the same Notebook as the highlighted destination). If a destination document of that name already exists, it will be irrevocably overwritten.

### Command-line mode

Command-line mode will only work if logon tokens have previously been obtained in interactive mode. It is intended to be used in scheduled tasks or batch files. 

Command line switches are as follows:

```
OneNoteToRM.exe -C -I <Input Document> -O <Output Document> -M <Mode>
```

- -C   indicates command line mode
- -M R: Remarkable to OneNote
	- -I is RM document name
	- -O is mandatory as name of Notebook to insert section into
- -M O: OneNote to Remarkable
	- -I use format Notebook/Section
	- -O is ignored

## Acknowledgements and references

### RMDoc

The format of ReMarkable .rmdoc files is not officially documented. I made heavy use of the knowldge uncovered by the following:

- Rick Lupton's RMScene - <https://github.com/ricklupton/rmscene/tree/main>
- ddvk's work - <https://github.com/ddvk>
- Excustic's further development of RMC, including the InkML exporter - <https://github.com/Excustic/rmc>

### OneNote (Graph) API

The native OneNote document format is so complex as to be unusable. However, apart from the complexity of setting up the OAuth 
for logon, the Graph API is relatively straightforward.
	
- see <https://learn.microsoft.com/en-us/graph/integrate-with-onenote> for API description.
- see <https://microsoft.github.io/cpprestsdk/classweb_1_1http_1_1http__request.html> for documentation on C++ REST API
- I also used the nlohmann/json parser for building the body of requests and managing the responses. See <https://json.nlohmann.me/api/basic_json/> for JSON docs
- I also used the pugixml C++ XML processing library for processing the HTML and InkML documents: <https://pugixml.org>