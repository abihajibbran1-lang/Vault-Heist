#include "frontend/UIManager.h"

// Diagnostic-only automation: submit the same controller commands as the UI.
// No algorithm or gameplay shortcut is used by the normal application path.
class CaptureCommands
{
public:
    static bool send(GameController& controller, const std::string& name,
                     const std::string& text = "", int nodeId = -1, bool finish = true)
    {
        PlayerAction action;
        action.name = name;
        action.text = text;
        action.nodeId = nodeId;
        OperationResult result = controller.performAction(action);
        if (finish)
        {
            controller.update(1000);
        }
        return result.accepted && result.success;
    }

    static bool pauseAt(GameController& controller, const std::string& type, bool after)
    {
        AnimationController& animation = controller.animation();
        animation.pause();
        while (animation.getEvent() != NULL && animation.getEvent()->type != type)
        {
            animation.next();
        }
        if (animation.getEvent() == NULL)
        {
            return false;
        }
        if (after)
        {
            animation.next();
        }
        return true;
    }
};

bool UIManager::prepareEvidence(const std::string& phase)
{
    if (phase == "LL" || phase == "RR" || phase == "LR" || phase == "RL")
    {
        bool single = phase == "LL" || phase == "RR";
        int captureRound = phase == "RR" || phase == "RL" ? 1 : 0;
        for (int round = 0; round <= captureRound; round++)
        {
            GameSnapshot current = controller.getSnapshot();
            int root = current.model.rootId;
            std::string kind = single ? (round == 0 ? "LL" : "RR") : (round == 0 ? "LR" : "RL");
            if (!CaptureCommands::send(controller, "SELECT_PIVOT", "", root) ||
                !CaptureCommands::send(controller, "ROTATION_CASE", kind, -1, !single || round < captureRound))
            {
                return false;
            }
            if (!single)
            {
                int child = kind == "LR" ? current.model.nodes[0].leftId : current.model.nodes[0].rightId;
                if (!CaptureCommands::send(controller, "SELECT_PIVOT", "", child) ||
                    !CaptureCommands::send(controller, "ROTATE_PRIMITIVE", kind == "LR" ? "LEFT" : "RIGHT") ||
                    !CaptureCommands::send(controller, "SELECT_PIVOT", "", root) ||
                    !CaptureCommands::send(controller, "ROTATE_PRIMITIVE", kind == "LR" ? "RIGHT" : "LEFT", -1, round < captureRound))
                {
                    return false;
                }
            }
        }
        notice = "Automated evidence: actual " + phase + " rotation playback frame";
        return CaptureCommands::pauseAt(controller, "ROTATE", true);
    }
    if (phase != "build" && phase != "extract" && phase != "heapify" && phase != "final")
    {
        return false;
    }
    if (!controller.heapReady())
    {
        if (!CaptureCommands::send(controller, "BUILD_ACTIVE_HEAP", "", -1, phase != "build"))
        {
            return false;
        }
    }
    if (phase == "build")
    {
        notice = "Automated evidence: real bottom-up build events";
        return CaptureCommands::pauseAt(controller, "DONE", false);
    }
    if (phase == "extract" || phase == "heapify")
    {
        if (!CaptureCommands::send(controller, "EXTRACT_SORTED", "", -1, phase == "heapify"))
        {
            return false;
        }
        if (phase == "heapify" && !CaptureCommands::send(controller, "HEAPIFY", "", -1, false))
        {
            return false;
        }
        notice = "Automated evidence: real " + phase + " events, sorted suffix retained";
        return CaptureCommands::pauseAt(controller, "DONE", false);
    }
    int steps = 0;
    while (controller.getState().status == "PLAYING" && steps++ < 200)
    {
        if (!CaptureCommands::send(controller, controller.heapReady() ? "EXTRACT_SORTED" : "HEAPIFY"))
        {
            return false;
        }
    }
    notice = "Automated evidence: the controller completed the full real sorting workflow";
    return controller.getState().completed;
}
