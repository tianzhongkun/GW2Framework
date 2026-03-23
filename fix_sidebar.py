import sys

path = r'C:\Users\Loong\.openclaw\workspace\GW2Framework\src\DX11Hook.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old = 'static void RenderSidebar()\n{\n    ImGui::BeginChild("##sidebar", ImVec2(220, 0), true);'

new = '''static void RenderSidebar()
{
    float maxTextW = 0.0f;
    for (int i = 0; i < IM_ARRAYSIZE(g_categories); ++i) {
        float w = ImGui::CalcTextSize(g_categories[i]).x;
        if (w > maxTextW) maxTextW = w;
    }
    float sidebarW = maxTextW + ImGui::GetStyle().WindowPadding.x * 2.0f + 48.0f;

    ImGui::BeginChild("##sidebar", ImVec2(sidebarW, 0), true);'''

if old in content:
    content = content.replace(old, new, 1)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    print('OK')
else:
    print('NOT_FOUND')
    # debug: show surrounding context
    idx = content.find('RenderSidebar')
    print(repr(content[idx:idx+120]))
