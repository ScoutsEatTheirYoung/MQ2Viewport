/*
 * MQ2Viewport - Exposes EverQuest camera data to the MQ2 TLO system.
 *
 * Basic members:
 *   /echo ${Viewport.X}          -- camera position X
 *   /echo ${Viewport.Y}          -- camera position Y
 *   /echo ${Viewport.Z}          -- camera position Z
 *   /echo ${Viewport.Heading}    -- degrees (0=N, 90=E, 180=S, 270=W)
 *   /echo ${Viewport.Pitch}      -- degrees (positive=up, ~85 max)
 *   /echo ${Viewport.Roll}       -- degrees
 *   /echo ${Viewport.FOV}        -- vertical field of view in degrees
 *   /echo ${Viewport.ScreenW}    -- render buffer width in pixels
 *   /echo ${Viewport.ScreenH}    -- render buffer height in pixels
 *   /echo ${Viewport}            -- "X=... Y=... Z=... H=... P=... FOV=..."
 *   /viewport                    -- prints all values to chat
 *
 * World-to-screen projection (Index = "worldX,worldY,worldZ"):
 *   /echo ${Viewport.Project[x,y,z]}         -- "sx,sy" or FALSE if behind camera
 *   /echo ${Viewport.ProjectClamped[x,y,z]}  -- "sx,sy", clamped to nearest screen edge
 *   /echo ${Viewport.ProjectX[x,y,z]}        -- screen X float, -1 if behind camera
 *   /echo ${Viewport.ProjectY[x,y,z]}        -- screen Y float, -1 if behind camera
 *   /echo ${Viewport.ProjectVisible[x,y,z]}  -- TRUE only if on screen and in front of camera
 */

#include <mq/Plugin.h>
#include "eqlib/graphics/CameraInterface.h"

using namespace mq::datatypes;
using namespace eqlib;

// EQ uses a 512-unit circle internally. 512 units = 360 degrees.
static constexpr float EQ_TO_DEG = 360.0f / 512.0f;

PLUGIN_VERSION(1.0);
PreSetup("MQ2Viewport");

// ── Viewport type ───────────────────────────────────────────────────────────

class MQ2ViewportType;
MQ2ViewportType* pViewportType = nullptr;

class MQ2ViewportType : public MQ2Type
{
public:
    enum class ViewportMembers
    {
        X,
        Y,
        Z,
        Heading,
        Pitch,
        Roll,
        FOV,
        ScreenW,
        ScreenH,
        Project,
        ProjectClamped,
        ProjectX,
        ProjectY,
        ProjectVisible,
    };

    MQ2ViewportType() : MQ2Type("Viewport")
    {
        ScopedTypeMember(ViewportMembers, X);
        ScopedTypeMember(ViewportMembers, Y);
        ScopedTypeMember(ViewportMembers, Z);
        ScopedTypeMember(ViewportMembers, Heading);
        ScopedTypeMember(ViewportMembers, Pitch);
        ScopedTypeMember(ViewportMembers, Roll);
        ScopedTypeMember(ViewportMembers, FOV);
        ScopedTypeMember(ViewportMembers, ScreenW);
        ScopedTypeMember(ViewportMembers, ScreenH);
        ScopedTypeMember(ViewportMembers, Project);
        ScopedTypeMember(ViewportMembers, ProjectClamped);
        ScopedTypeMember(ViewportMembers, ProjectX);
        ScopedTypeMember(ViewportMembers, ProjectY);
        ScopedTypeMember(ViewportMembers, ProjectVisible);
    }

    bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
    {
        if (!pDisplay || !pDisplay->pCamera)
            return false;

        MQTypeMember* pMember = MQ2ViewportType::FindMember(Member);
        if (!pMember)
            return false;

        CCameraInterface* cam = pDisplay->pCamera;

        switch (static_cast<ViewportMembers>(pMember->ID))
        {
        case ViewportMembers::X:
            Dest.Float = cam->GetX();
            Dest.Type = pFloatType;
            return true;

        case ViewportMembers::Y:
            Dest.Float = cam->GetY();
            Dest.Type = pFloatType;
            return true;

        case ViewportMembers::Z:
            Dest.Float = cam->GetZ();
            Dest.Type = pFloatType;
            return true;

        case ViewportMembers::Heading:
            Dest.Float = cam->GetHeading() * EQ_TO_DEG;
            Dest.Type = pFloatType;
            return true;

        case ViewportMembers::Pitch:
            Dest.Float = cam->GetPitch() * EQ_TO_DEG;
            Dest.Type = pFloatType;
            return true;

        case ViewportMembers::Roll:
            Dest.Float = cam->GetRoll() * EQ_TO_DEG;
            Dest.Type = pFloatType;
            return true;

        case ViewportMembers::FOV:
        {
            CCamera* ccam = cam->AsCamera();
            if (!ccam) return false;
            // halfViewAngle is already in degrees; full FOV = half * 2
            Dest.Float = ccam->halfViewAngle * 2.0f;
            Dest.Type = pFloatType;
            return true;
        }

        case ViewportMembers::ScreenW:
            Dest.Float = cam->GetRenderBufferWidth();
            Dest.Type = pFloatType;
            return true;

        case ViewportMembers::ScreenH:
            Dest.Float = cam->GetRenderBufferHeight();
            Dest.Type = pFloatType;
            return true;

        case ViewportMembers::Project:
        {
            float wx, wy, wz;
            if (!Index || sscanf_s(Index, "%f,%f,%f", &wx, &wy, &wz) != 3)
                return false;
            float sx, sy;
            if (!cam->ProjectWorldCoordinatesToScreen(CVector3(wx, wy, wz), sx, sy))
                return false;
            static char szProject[64];
            sprintf_s(szProject, "%.2f,%.2f", sx, sy);
            Dest.Ptr = szProject;
            Dest.Type = pStringType;
            return true;
        }

        case ViewportMembers::ProjectClamped:
        {
            float wx, wy, wz;
            if (!Index || sscanf_s(Index, "%f,%f,%f", &wx, &wy, &wz) != 3)
                return false;
            float sx, sy;
            // Project first (result ignored — we always clamp)
            cam->ProjectWorldCoordinatesToScreen(CVector3(wx, wy, wz), sx, sy);
            float cx, cy;
            cam->GetClampedScreenCoordinates(cx, cy, sx, sy);
            static char szProjectClamped[64];
            sprintf_s(szProjectClamped, "%.2f,%.2f", cx, cy);
            Dest.Ptr = szProjectClamped;
            Dest.Type = pStringType;
            return true;
        }

        case ViewportMembers::ProjectX:
        {
            float wx, wy, wz;
            if (!Index || sscanf_s(Index, "%f,%f,%f", &wx, &wy, &wz) != 3)
                return false;
            float sx, sy;
            if (!cam->ProjectWorldCoordinatesToScreen(CVector3(wx, wy, wz), sx, sy))
            {
                Dest.Float = -1.0f;
                Dest.Type = pFloatType;
                return true;
            }
            Dest.Float = sx;
            Dest.Type = pFloatType;
            return true;
        }

        case ViewportMembers::ProjectY:
        {
            float wx, wy, wz;
            if (!Index || sscanf_s(Index, "%f,%f,%f", &wx, &wy, &wz) != 3)
                return false;
            float sx, sy;
            if (!cam->ProjectWorldCoordinatesToScreen(CVector3(wx, wy, wz), sx, sy))
            {
                Dest.Float = -1.0f;
                Dest.Type = pFloatType;
                return true;
            }
            Dest.Float = sy;
            Dest.Type = pFloatType;
            return true;
        }

        case ViewportMembers::ProjectVisible:
        {
            float wx, wy, wz;
            if (!Index || sscanf_s(Index, "%f,%f,%f", &wx, &wy, &wz) != 3)
                return false;
            float sx, sy;
            bool inFront = cam->ProjectWorldCoordinatesToScreen(CVector3(wx, wy, wz), sx, sy);
            float w = cam->GetRenderBufferWidth();
            float h = cam->GetRenderBufferHeight();
            Dest.DWord = inFront && sx >= 0.0f && sx <= w && sy >= 0.0f && sy <= h ? 1 : 0;
            Dest.Type = pBoolType;
            return true;
        }
        }

        return false;
    }

    bool ToString(MQVarPtr VarPtr, char* Destination) override
    {
        if (!pDisplay || !pDisplay->pCamera)
        {
            strcpy_s(Destination, MAX_STRING, "NULL");
            return true;
        }

        CCameraInterface* cam = pDisplay->pCamera;
        float fov = 0.f;
        if (CCamera* ccam = cam->AsCamera())
            fov = ccam->halfViewAngle * 2.0f;

        sprintf_s(Destination, MAX_STRING, "X=%.4f Y=%.4f Z=%.4f H=%.2f P=%.2f FOV=%.1f",
            cam->GetX(), cam->GetY(), cam->GetZ(),
            cam->GetHeading() * EQ_TO_DEG, cam->GetPitch() * EQ_TO_DEG, fov);
        return true;
    }

    static bool dataViewport(const char* szIndex, MQTypeVar& Ret)
    {
        Ret.DWord = 1;
        Ret.Type = pViewportType;
        return true;
    }
};

// ── /viewport command ───────────────────────────────────────────────────────

static void CmdViewport(PlayerClient* /*pChar*/, const char* /*szLine*/)
{
    if (!pDisplay || !pDisplay->pCamera)
    {
        WriteChatf("\a[MQ2Viewport]\ax No camera available.");
        return;
    }

    CCameraInterface* cam = pDisplay->pCamera;

    float fov = 0.f;
    if (CCamera* ccam = cam->AsCamera())
        fov = ccam->halfViewAngle * 2.0f;

    WriteChatf("\a[MQ2Viewport]\ax --- Viewport Info ---");
    WriteChatf("  Position : \ayX=%.4f  Y=%.4f  Z=%.4f", cam->GetX(), cam->GetY(), cam->GetZ());
    WriteChatf("  Angles   : \ayHeading=%.2f\xb0  Pitch=%.2f\xb0  Roll=%.2f\xb0",
        cam->GetHeading() * EQ_TO_DEG, cam->GetPitch() * EQ_TO_DEG, cam->GetRoll() * EQ_TO_DEG);
    WriteChatf("  FOV      : \ay%.2f\xb0", fov);
    WriteChatf("  Screen   : \ay%.0f x %.0f", cam->GetRenderBufferWidth(), cam->GetRenderBufferHeight());
}

// ── Plugin lifecycle ────────────────────────────────────────────────────────

PLUGIN_API void InitializePlugin()
{
    DebugSpewAlways("MQ2Viewport::InitializePlugin");
    pViewportType = new MQ2ViewportType();
    AddMQ2Type(*pViewportType);
    AddMQ2Data("Viewport", MQ2ViewportType::dataViewport);
    AddCommand("/viewport", CmdViewport);
    WriteChatf("\a[MQ2Viewport]\ax loaded - type \ay/viewport\ax for info");
}

PLUGIN_API void ShutdownPlugin()
{
    DebugSpewAlways("MQ2Viewport::ShutdownPlugin");
    RemoveCommand("/viewport");
    RemoveMQ2Data("Viewport");
    RemoveMQ2Type(*pViewportType);
    delete pViewportType;
    pViewportType = nullptr;
}
