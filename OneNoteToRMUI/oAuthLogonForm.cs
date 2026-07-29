using Microsoft.Extensions.Configuration.Ini;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace OneNoteToRMUI
{
    public partial class oAuthLogonForm : Form
    {
        private string RedirectURIHost = "";
        private string RedirectURIPath = "";

        public oAuthLogonForm()
        {
            InitializeComponent();
        }

        private void oAuthLogonForm_Load(object sender, EventArgs e)
        {
            string EntraAppID = DllWrapper.GetIniSetting("OneNote", "EntraAppID");
            string TenantID = DllWrapper.GetIniSetting("OneNote", "TenantID");
            string RedirectURI = DllWrapper.GetIniSetting("OneNote", "RedirectURI");
            RedirectURIHost = DllWrapper.GetIniSetting("OneNote", "RedirectURIHost");
            RedirectURIPath = DllWrapper.GetIniSetting("OneNote", "RedirectURIPath");
            string EndpointHost = DllWrapper.GetIniSetting("OneNote", "EndpointHost");
            string OAuthEndpoint = DllWrapper.GetIniSetting("OneNote", "OAuthEndpoint");

            UriBuilder URI = new UriBuilder
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

        private void webView_NavigationStarting(object sender, Microsoft.Web.WebView2.Core.CoreWebView2NavigationStartingEventArgs e)
        {
            //listBox1.Items.Add("Nav Starting: ID=" + e.NavigationId.ToString() + " Kind=" + e.NavigationKind.ToString() + " URI=" + e.Uri.ToString());
            UriBuilder URI = new UriBuilder(e.Uri);

            if (URI.Host == RedirectURIHost && URI.Path == RedirectURIPath)
            {
                //listBox1.Items.Add("NAV: Arrived! [" + URI.Query + "]");
                //                    PostMessage(hWnd, WM_DONELOGINTOMS, NULL, (LPARAM)QueryString);
                string Q = URI.Query;
                if (Q[0] =='?')
                    Q = Q.Substring(1);
                DllWrapper.DLLSetToken(DllWrapper.PageType.WINDOW_ONE_PAGE, Q);
                e.Cancel = true; // Don't actually go there
                this.Close();
            }
        }
    }
}
