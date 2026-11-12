#include "api.h"
#include "../../runtime/native/input.h"
#include "../../runtime/native/local_actor.h"
#include "../build-menu-framework/sdk/construction_unity.h"
#include "shop_signal.h"
#include "repeat_contract.h"
#include "repeat_unity.h"

static struct {
    PTR log, world_field, restoring, focused, typing, blocked, first, frame;
    PTR joined, host, local, game;
    void (*set_field)(PTR, PTR, void *);
    PTR (*foreground)(void);
    DWORD (*pid)(PTR, DWORD *);
    DRoot world, walker;
    int last_frame, eligible, disabled;
    DWORD thread;
    MoonRepeatFields tickets;
} bm;

static void bevent(const char *s) {
    char b[512], *p = cat(b, "{\"event\":\"");
    p = cat(p, s); p = cat(p, "\"}\n");
    DWORD n = 0;
    WriteFile(bm.log, b, (DWORD)(p-b), &n, 0);
    FlushFileBuffers(bm.log);
}
static int bhost(void) {
    int ok = 1;
    if (!boolean(bm.joined, 0, &ok))
        return ok;
    PTR host = call(bm.host, 0, 0, &ok), local = call(bm.local, 0, 0, &ok);
    U64 a = 0, b = 0;
    if (host) dcopy(&a, unbox(host), 8);
    if (local) dcopy(&b, unbox(local), 8);
    return ok && a && a == b;
}
static int beligible(PTR walker, PTR world) {
    int ok = 1;
    DWORD pid = 0;
    PTR window = bm.foreground();
    if (window) bm.pid(window, &pid);
    return moon_local_actor_admit(walker) && world && !boolean(bm.restoring, world, &ok) && bhost() &&
           pid == GetCurrentProcessId() && boolean(bm.focused, 0, &ok) &&
           !boolean(bm.typing, 0, &ok) && !boolean(bm.blocked, walker, &ok) &&
           boolean(bm.first, walker, &ok) && ok;
}
static int bframe(Context *context) {
    if (bm.disabled) return 0;
    DWORD thread = GetCurrentThreadId();
    if (bm.thread && bm.thread != thread) return 0;
    PTR local = 0;
    if (!moon_local_actor_read(&local) || !local) {
        bm.eligible = 0;
        bm.last_frame = -1;
        return 0;
    }
    /* Generic Update observes every walker. Remote callbacks cannot replace
     * the local owner, pin its thread or consume its frame. A newly observed
     * local identity does revoke eligibility of an old cached actor. */
    PTR walker = (PTR)context->rcx, world = 0;
    if (walker != local) {
        if (local != dget(bm.walker)) bm.eligible = 0;
        return 0;
    }
    if (!bm.thread) bm.thread = thread;
    field_value(walker, bm.world_field, &world);
    if (world != dget(bm.world) || walker != dget(bm.walker)) {
        ddrop(&bm.world); ddrop(&bm.walker);
        bm.world = dhold(world); bm.walker = dhold(walker);
        bm.eligible = 0;
        bm.last_frame = -1;
        if (world && (!bm.world || !bm.walker)) {
            ddrop(&bm.world); ddrop(&bm.walker);
            bevent("session-root-unavailable");
        }
        return -1;
    }
    int ok = 1, frame = integer(bm.frame, 0, &ok);
    if (!ok) {bm.eligible = 0; bm.last_frame = -1; return 0;}
    if (frame == bm.last_frame) return 0;
    bm.last_frame = frame;
    bm.eligible = beligible(walker, world);
    return 1;
}
static PTR bpinned(const char *name, const char *signature, DWORD token, unsigned rva) {
    PTR m = dmethod("MoonCleaners", "MoonDepotBuilder", name, signature);
    return m && method_token(m) == token && *(PTR *)m == (unsigned char *)bm.game + rva ? m : 0;
}
static int btickets(void) {
    PTR klass = find_class("MoonCleaners", "MoonDepotBuilder");
    PTR (*field_type)(PTR) = (PTR(*)(PTR))GetProcAddress(bm.game, "il2cpp_field_get_type");
    unsigned (*offset)(PTR) = (unsigned(*)(PTR))GetProcAddress(bm.game, "il2cpp_field_get_offset");
    unsigned (*flags)(PTR) = (unsigned(*)(PTR))GetProcAddress(bm.game, "il2cpp_field_get_flags");
    if (!klass || !field_type || !offset || !flags) return 0;
    static const struct {const char *name, *type; unsigned offset;} expected[] = {
        {"placementCheck", "MoonCleaners.MoonWorld.DepotPlacementCheck", 1024},
        {"structurePlan", "MoonCleaners.MoonWorld.StructurePlan", 1096},
        {"lastValid", "MoonCleaners.MoonWorld.DepotPlacementCheck", 1136},
        {"shown", "MoonCleaners.MoonWorld.DepotPlacementCheck", 1144},
        {"valid", "System.Boolean", 1009},
        {"looksValid", "System.Boolean", 1152},
        {"nextCheck", "System.Single", 1020}
    };
    PTR *targets[] = {&bm.tickets.check, &bm.tickets.plan, &bm.tickets.last_valid, &bm.tickets.shown,
                      &bm.tickets.valid, &bm.tickets.looks_valid, &bm.tickets.next_check};
    for (unsigned i = 0; i < sizeof(expected)/sizeof(*expected); i++) {
        PTR field = get_field(klass, expected[i].name);
        if (!field || (flags(field) & 0x10) || offset(field) != expected[i].offset) return 0;
        char *type = da.type_name(field_type(field));
        int matches = type && deq(type, expected[i].type);
        if (type) da.free(type);
        if (!matches) return 0;
        *targets[i] = field;
    }
    return 1;
}
static int bstart(PTR domain, PTR game, const char *id) {
    if (!bind_runtime(domain, game) || !dapi(game) || !moon_local_actor_bind()) return 0;
    bm.game = game;
    char folder[1000], path[1200];
    DWORD n = GetEnvironmentVariableA("MOONLAB_EVIDENCE", folder, sizeof(folder));
    if (!n || n >= sizeof(folder)) return 0;
    char *p = cat(path, folder); p = cat(p, "\\"); p = cat(p, id); cat(p, ".jsonl");
    bm.log = CreateFileA(path, 0x40000000, 1, 0, 1, 0x80, 0);
    if (bm.log == (PTR)(U64)-1) return 0;
#define BM(F,NS,T,N,S) bm.F=dmethod(NS,T,N,S);if(!bm.F){bevent(T "::" N);return 0;}
    BM(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");
    BM(focused,"UnityEngine","Application","get_isFocused","");
    BM(typing,"MoonCleaners","MoonInput","get_Typing","");
    BM(blocked,"MoonCleaners","MoonWalker","get_MenuBlocked","");
    BM(first,"MoonCleaners","MoonWalker","get_IsFirstPerson","");
    BM(frame,"UnityEngine","Time","get_frameCount","");
    BM(joined,"MoonCleaners","MoonCrew","get_Joined","");
    BM(host,"MoonCleaners","MoonCrew","get_HostId","");
    BM(local,"Steamworks","SteamUser","GetSteamID","");
#undef BM
    bm.world_field = get_field(find_class("MoonCleaners", "MoonWalker"), "world");
    bm.set_field = (void(*)(PTR,PTR,void *))GetProcAddress(game, "il2cpp_field_set_value");
    PTR user = LoadLibraryA("user32.dll");
    bm.foreground = (PTR(*)(void))GetProcAddress(user, "GetForegroundWindow");
    bm.pid = (DWORD(*)(PTR,DWORD *))GetProcAddress(user, "GetWindowThreadProcessId");
    bm.last_frame = -1;
    (void)moon_shop_unlisten;
    return bm.world_field && bm.set_field && bm.foreground && bm.pid && btickets() &&
           moon_shop_listen(id);
}
