// =====================================================================
// LootDebug — F4 in-building loot inspector for COM (ChernarusPlus).
// Output: chat (GetGame().Chat) + on-screen hint + script.log.
// Parses mapgroupproto.xml + types.xml at module Init.
// =====================================================================

class LD_Container
{
    string name;
    int lootmax;
    int points;
    ref array<string> categories = new array<string>;
    ref array<string> tags = new array<string>;
}

class LD_ProtoGroup
{
    int lootmax;
    ref array<string> usages = new array<string>;
    ref array<ref LD_Container> containers = new array<ref LD_Container>;
}

class LD_Type
{
    int nominal;
    int min;
    int lifetime;
    int restock;
    ref array<string> categories = new array<string>;
    ref array<string> usages = new array<string>;
    ref array<string> tags = new array<string>;
    ref array<string> values = new array<string>;
}

class LootDebug extends Module
{
    protected ref map<string, ref LD_ProtoGroup> m_Proto;
    protected ref map<string, ref LD_Type> m_Types;
    protected bool m_Ready = false;
    protected string m_LastReport = "(no data — press LD button in a building)";

    string GetLastReport() { return m_LastReport; }

    // ChernarusPlus-specific. Mirror copies override these in their LootDebug.c.
    protected string PROTO_PATH = "$CurrentDir:Missions\\DayZCommunityOfflineMode.Enoch\\mapgroupproto.xml";
    protected string TYPES_PATH = "$CurrentDir:Missions\\DayZCommunityOfflineMode.Enoch\\db\\types.xml";

    void LootDebug() {}
    void ~LootDebug() {}

    override void Init()
    {
        super.Init();

        Print("[LootDebug] Init starting");
        m_Proto = new map<string, ref LD_ProtoGroup>;
        m_Types = new map<string, ref LD_Type>;
        ParseProto(PROTO_PATH);
        ParseTypes(TYPES_PATH);
        m_Ready = true;
        Print(string.Format("[LootDebug] Ready. Proto groups: %1, Types: %2. Use LD toolbar button.", m_Proto.Count(), m_Types.Count()));
    }

    override void RegisterKeyMouseBindings()
    {
        // F4 binding removed — triggered from ComEditor toolbar (LD button) instead.
    }

    // ----- Helpers -----
    protected string GetAttr(string line, string attr)
    {
        string needle = attr + "=\"";
        int start = line.IndexOf(needle);
        if (start < 0) return "";
        start += needle.Length();
        int end = line.IndexOfFrom(start, "\"");
        if (end < 0) return "";
        return line.Substring(start, end - start);
    }

    protected int ToIntSafe(string s)
    {
        if (s == "") return 0;
        return s.ToInt();
    }

    protected string ExtractTagValue(string line, string tag)
    {
        string open = "<" + tag + ">";
        string close = "</" + tag + ">";
        int s = line.IndexOf(open);
        if (s < 0) return "";
        s += open.Length();
        int e = line.IndexOfFrom(s, close);
        if (e < 0) return "";
        return line.Substring(s, e - s);
    }

    // ----- Parse mapgroupproto.xml -----
    protected void ParseProto(string path)
    {
        Print("[LootDebug] ParseProto opening: " + path);
        FileHandle fh = OpenFile(path, FileMode.READ);
        if (!fh) { Print("[LootDebug] Could not open " + path); return; }
        Print("[LootDebug] ParseProto opened OK");

        string line;
        LD_ProtoGroup curGroup = null;
        LD_Container curContainer = null;
        string curGroupName = "";
        int linesRead = 0;
        int groupsFound = 0;

        while (FGets(fh, line) >= 0)
        {
            linesRead++;
            if (line.Contains("<group "))
            {
                groupsFound++;
                curGroupName = GetAttr(line, "name");
                curGroup = new LD_ProtoGroup;
                curGroup.lootmax = ToIntSafe(GetAttr(line, "lootmax"));
                continue;
            }
            if (line.Contains("</group>"))
            {
                if (curGroup && curGroupName != "") m_Proto.Set(curGroupName, curGroup);
                curGroup = null; curGroupName = "";
                continue;
            }
            if (line.Contains("<container "))
            {
                curContainer = new LD_Container;
                curContainer.name = GetAttr(line, "name");
                curContainer.lootmax = ToIntSafe(GetAttr(line, "lootmax"));
                continue;
            }
            if (line.Contains("</container>"))
            {
                if (curGroup && curContainer) curGroup.containers.Insert(curContainer);
                curContainer = null;
                continue;
            }
            if (line.Contains("<point ") && curContainer)
            {
                curContainer.points++;
                continue;
            }
            if (line.Contains("<usage ") && curGroup && !curContainer)
            {
                curGroup.usages.Insert(GetAttr(line, "name"));
                continue;
            }
            if (line.Contains("<category ") && curContainer)
            {
                curContainer.categories.Insert(GetAttr(line, "name"));
                continue;
            }
            if (line.Contains("<tag ") && curContainer)
            {
                curContainer.tags.Insert(GetAttr(line, "name"));
            }
        }
        Print(string.Format("[LootDebug] ParseProto done. linesRead=%1 groupsFound=%2 mapCount=%3", linesRead, groupsFound, m_Proto.Count()));
        CloseFile(fh);
    }

    // ----- Parse types.xml -----
    protected void ParseTypes(string path)
    {
        FileHandle fh = OpenFile(path, FileMode.READ);
        if (!fh) { Print("[LootDebug] Could not open " + path); return; }

        string line;
        LD_Type curType = null;
        string curTypeName = "";

        while (FGets(fh, line) >= 0)
        {
            if (line.Contains("<type "))
            {
                curTypeName = GetAttr(line, "name");
                curType = new LD_Type;
                continue;
            }
            if (line.Contains("</type>"))
            {
                if (curType && curTypeName != "") m_Types.Set(curTypeName, curType);
                curType = null; curTypeName = "";
                continue;
            }
            if (!curType) continue;

            if (line.Contains("<nominal>"))
                curType.nominal = ToIntSafe(ExtractTagValue(line, "nominal"));
            else if (line.Contains("<min>"))
                curType.min = ToIntSafe(ExtractTagValue(line, "min"));
            else if (line.Contains("<lifetime>"))
                curType.lifetime = ToIntSafe(ExtractTagValue(line, "lifetime"));
            else if (line.Contains("<restock>"))
                curType.restock = ToIntSafe(ExtractTagValue(line, "restock"));
            else if (line.Contains("<category "))
                curType.categories.Insert(GetAttr(line, "name"));
            else if (line.Contains("<usage "))
                curType.usages.Insert(GetAttr(line, "name"));
            else if (line.Contains("<tag "))
                curType.tags.Insert(GetAttr(line, "name"));
            else if (line.Contains("<value "))
                curType.values.Insert(GetAttr(line, "name"));
        }
        CloseFile(fh);
    }

    // ----- LD button callback: dump info -----
    void DumpLootInfo()
    {
        // Toggle: if menu open, close.
        UIScriptedMenu existing = GetGame().GetUIManager().FindMenu( LootDebugMenu.MENU_ID );
        if ( existing )
        {
            GetGame().GetUIManager().CloseMenu( LootDebugMenu.MENU_ID );
            return;
        }

        if (!m_Ready) { m_LastReport = "[LootDebug] Not ready yet."; OpenMenu(); return; }

        PlayerBase player = PlayerBase.Cast(COM_GetPB());
        if (!player) { Print("[LootDebug] No local player."); return; }

        vector pPos = player.GetPosition();

        // Find nearest Building within 25m
        array<Object> objects = new array<Object>;
        array<CargoBase> cargos = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D(pPos, 25.0, objects, cargos);

        Building nearest = null;
        float bestDist = 1e9;
        foreach (Object o : objects)
        {
            Building b = Building.Cast(o);
            if (!b) continue;
            float d = vector.Distance(b.GetWorldPosition(), pPos);
            if (d < bestDist) { bestDist = d; nearest = b; }
        }

        if (!nearest)
        {
            m_LastReport = "[LootDebug] No building within 25m of player position.";
            OpenMenu();
            return;
        }

        string cls = nearest.GetType();
        vector bpos = nearest.GetWorldPosition();
        float scanR = 30.0;
        string report = "";

        report += string.Format("Building: %1\n", cls);
        report += string.Format("Distance: %1 m   Pos: %2\n", bestDist, bpos.ToString());
        report += "------------------------------------------------------------\n";

        LD_ProtoGroup pg = m_Proto.Get(cls);
        if (pg)
        {
            report += string.Format("lootmax: %1   usages: [%2]   containers: %3\n",
                pg.lootmax, JoinList(pg.usages, ","), pg.containers.Count());
            foreach (LD_Container c : pg.containers)
            {
                report += string.Format("  - %1  [max=%2  pts=%3]\n     cats: [%4]\n     tags: [%5]\n",
                    c.name, c.lootmax, c.points,
                    JoinList(c.categories, ","), JoinList(c.tags, ","));
            }
        }
        else
        {
            report += "(no mapGroupProto entry — building has no defined loot points)\n";
        }
        report += "------------------------------------------------------------\n";

        // Scan nearby items
        array<Object> items = new array<Object>;
        array<CargoBase> dummy = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D(bpos, scanR, items, dummy);

        // Collect tier set from currently-spawned items (proxy for building tier).
        ref array<string> spawnedTiers = new array<string>;
        int noTierCount = 0;

        int itemCount = 0;
        string itemBlock = "";
        foreach (Object io : items)
        {
            ItemBase ib = ItemBase.Cast(io);
            if (!ib) continue;
            if (ib.GetHierarchyRootPlayer()) continue;
            itemCount++;

            string itemCls = ib.GetType();
            LD_Type t = m_Types.Get(itemCls);
            if (t)
            {
                if (t.values.Count() == 0) noTierCount++;
                else
                {
                    foreach (string v : t.values)
                        if (spawnedTiers.Find(v) < 0) spawnedTiers.Insert(v);
                }
            }

            if (itemCount <= 50)
            {
                float dist = vector.Distance(ib.GetWorldPosition(), bpos);
                string flags;
                if (t)
                    flags = string.Format("nom=%1 min=%2 cat=[%3] use=[%4] val=[%5]",
                        t.nominal, t.min,
                        JoinList(t.categories, ","), JoinList(t.usages, ","), JoinList(t.values, ","));
                else
                    flags = "(no types.xml entry)";
                itemBlock += string.Format("  * %1  @%2m\n     %3\n", itemCls, dist, flags);
            }
        }

        // Tier inference + spawnable candidate breakdown (only if proto known).
        if (pg)
        {
            report += string.Format("Tier (from spawned items): [%1]   no-tier items: %2\n",
                JoinList(spawnedTiers, ","), noTierCount);

            // Build union of container categories.
            ref array<string> allCats = new array<string>;
            foreach (LD_Container cc : pg.containers)
                foreach (string cat : cc.categories)
                    if (allCats.Find(cat) < 0) allCats.Insert(cat);

            // Walk types: count those whose usage intersects building.usages,
            // bucket by tier and by category-match.
            int matchUsage = 0, matchUsageAndCat = 0;
            ref map<string,int> tierBuckets = new map<string,int>;
            tierBuckets.Set("Tier1", 0); tierBuckets.Set("Tier2", 0);
            tierBuckets.Set("Tier3", 0); tierBuckets.Set("Tier4", 0);
            tierBuckets.Set("Unique", 0); tierBuckets.Set("(none)", 0);

            foreach (string typeName, LD_Type tp : m_Types)
            {
                if (!ListsIntersect(tp.usages, pg.usages)) continue;
                matchUsage++;
                if (ListsIntersect(tp.categories, allCats)) matchUsageAndCat++;

                if (tp.values.Count() == 0)
                {
                    tierBuckets.Set("(none)", tierBuckets.Get("(none)") + 1);
                }
                else
                {
                    foreach (string vv : tp.values)
                    {
                        if (tierBuckets.Contains(vv))
                            tierBuckets.Set(vv, tierBuckets.Get(vv) + 1);
                    }
                }
            }

            report += string.Format("Spawnable types matching usage [%1]: %2 total  (also matching a container category: %3)\n",
                JoinList(pg.usages, ","), matchUsage, matchUsageAndCat);
            report += string.Format("  By tier flag:  Tier1=%1  Tier2=%2  Tier3=%3  Tier4=%4  Unique=%5  (no tier)=%6\n",
                tierBuckets.Get("Tier1"), tierBuckets.Get("Tier2"), tierBuckets.Get("Tier3"),
                tierBuckets.Get("Tier4"), tierBuckets.Get("Unique"), tierBuckets.Get("(none)"));
            report += string.Format("  Container category union: [%1]\n", JoinList(allCats, ","));
            report += "------------------------------------------------------------\n";
        }

        report += string.Format("Items currently in %1m sphere (excl. player inventory): %2\n", scanR, itemCount);
        report += itemBlock;

        // Mirror to log for grep history.
        Print("==================================================================");
        Print("[LootDebug]\n" + report);
        Print("==================================================================");

        m_LastReport = report;
        OpenMenu();
    }

    protected void OpenMenu()
    {
        UIMenuPanel parent = GetGame().GetUIManager().GetMenu();
        UIScriptedMenu m = GetGame().GetUIManager().EnterScriptedMenu( LootDebugMenu.MENU_ID, parent );
        Print("[LootDebug] EnterScriptedMenu returned " + (m != NULL) + " parent=" + (parent != NULL));
    }

    protected string JoinList(array<string> list, string sep)
    {
        string r = "";
        for (int i = 0; i < list.Count(); i++)
        {
            if (i > 0) r += sep;
            r += list[i];
        }
        return r;
    }

    protected bool ListsIntersect(array<string> a, array<string> b)
    {
        if (!a || !b) return false;
        for (int i = 0; i < a.Count(); i++)
            if (b.Find(a[i]) >= 0) return true;
        return false;
    }
}
