/*******************************************************************************

    oAuthLoginForm.cs

    Popup form to host browser control to perform Microsoft oAuth Login
    Loads login page details from .INI file, displays logon form, checks 
        for redirect on login, pulls token from redirect string and passes
        it to the DLL

    (C) David Poirier 2026

********************************************************************************/

namespace OneNoteToRMUI
{
    public partial class OAuthLogonForm : Form
    {
        private string RedirectURIHost = "";
        private string RedirectURIPath = "";

        public OAuthLogonForm()
        {
            InitializeComponent();
        }

        private void OAuthLogonForm_Load(object sender, EventArgs e)
        {
            string EntraAppID = DllWrapper.GetIniSetting("OneNote", "EntraAppID");
            string TenantID = DllWrapper.GetIniSetting("OneNote", "TenantID");
            string RedirectURI = DllWrapper.GetIniSetting("OneNote", "RedirectURI");
            RedirectURIHost = DllWrapper.GetIniSetting("OneNote", "RedirectURIHost");
            RedirectURIPath = DllWrapper.GetIniSetting("OneNote", "RedirectURIPath");
            string EndpointHost = DllWrapper.GetIniSetting("OneNote", "EndpointHost");
            string OAuthEndpoint = DllWrapper.GetIniSetting("OneNote", "OAuthEndpoint");

            UriBuilder URI = new()
            {
                Scheme = "https",
                Host = EndpointHost,
                Path = TenantID
            };
            URI.Path += "/" + OAuthEndpoint;

            URI.Query = "client_id=" + EntraAppID;
            URI.Query += "&response_type=code";
            URI.Query += "&redirect_uri=" + RedirectURI;
            URI.Query += "&response_mode=query";
            //    URI.append_query(L"scope", L"https://graph.microsoft.com/.default offline_access notes.Create notes.ReadWrite", true);
            URI.Query += "&scope=offline_access Notes.Create Notes.Read Notes.ReadWrite Notes.Read.All Notes.ReadWrite.All";

            webView.Source = URI.Uri;
        }

        private void WebView_NavigationStarting(object sender, Microsoft.Web.WebView2.Core.CoreWebView2NavigationStartingEventArgs e)
        {
            //listBox1.Items.Add("Nav Starting: ID=" + e.NavigationId.ToString() + " Kind=" + e.NavigationKind.ToString() + " URI=" + e.Uri.ToString());
            UriBuilder URI = new(e.Uri);

            if (URI.Host == RedirectURIHost && URI.Path == RedirectURIPath)
            {
                //listBox1.Items.Add("NAV: Arrived! [" + URI.Query + "]");
                //                    PostMessage(hWnd, WM_DONELOGINTOMS, NULL, (LPARAM)QueryString);
                string Q = URI.Query;
                if (Q[0] =='?')
                    Q = Q[1..];
                DllWrapper.DLLSetToken(DllWrapper.PageType.WINDOW_ONE_PAGE, Q);
                e.Cancel = true; // Don't actually go there
                this.Close();
            }
        }
    }
}
