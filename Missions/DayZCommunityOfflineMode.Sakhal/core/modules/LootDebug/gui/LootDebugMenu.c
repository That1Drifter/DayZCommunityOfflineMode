// UIScriptedMenu version. Opened via UIManager so input/cursor/close handling
// goes through the standard menu pipeline (clicks reach OnClick reliably).

class LootDebugMenu extends UIScriptedMenu
{
    static const int MENU_ID = 134701;

    protected MultilineTextWidget m_Info;
    protected ButtonWidget m_BtnClose;

    protected string LAYOUT_PATH = "$CurrentDir:Missions\\DayZCommunityOfflineMode.Sakhal\\core\\modules\\LootDebug\\gui\\layouts\\LootDebugMenu.layout";

    void LootDebugMenu()
    {
        SetID(MENU_ID);
    }

    override int GetID()
    {
        return MENU_ID;
    }

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets( LAYOUT_PATH );
        if ( !layoutRoot )
        {
            Print("[LootDebug] FAILED to load layout: " + LAYOUT_PATH);
            return null;
        }

        m_Info = MultilineTextWidget.Cast( layoutRoot.FindAnyWidget("infotext") );
        m_BtnClose = ButtonWidget.Cast( layoutRoot.FindAnyWidget("btn_close") );

        // Populate text from the LootDebug module's last report.
        #ifdef MODULE_LOOT_DEBUG
        LootDebug ld = LootDebug.Cast( COM_GetModuleManager().GetModule( LootDebug ) );
        if ( ld && m_Info ) m_Info.SetText( ld.GetLastReport() );
        #endif

        return layoutRoot;
    }

    override void OnShow()
    {
        super.OnShow();
        GetGame().GetUIManager().ShowUICursor( true );
    }

    override void OnHide()
    {
        super.OnHide();
    }

    override void LockControls()
    {
        super.LockControls();
        GetGame().GetInput().ChangeGameFocus( 1 );
    }

    override void UnlockControls()
    {
        super.UnlockControls();
        GetGame().GetInput().ChangeGameFocus( -1 );
    }

    override bool OnClick( Widget w, int x, int y, int button )
    {
        super.OnClick( w, x, y, button );
        if ( w == m_BtnClose )
        {
            Close();
            return true;
        }
        return false;
    }

    override bool OnKeyPress( Widget w, int x, int y, int key )
    {
        if ( key == KeyCode.KC_ESCAPE )
        {
            Close();
            return true;
        }
        return false;
    }
}
