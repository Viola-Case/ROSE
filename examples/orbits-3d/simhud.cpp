#line 2 "examples/orbits/simhud.cpp"
/**

  @file       simhud.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "simhud.h"
#include "orbitcamera.h"
#include "pointcloud.h"
#include "simcommon.h"
#include "tracer.h"

#include <imgui.h>
#include <cfloat>

using namespace ROSE;

namespace {
  constexpr double fpsRefreshInterval { 0.25 };

  void LabelledValue(const char *_label, const String &_value) noexcept {
    ImGui::TextUnformatted(_label);
    ImGui::SameLine(190.f);
    ImGui::TextUnformatted(_value.c_str());
  }

  /*! Drift is signed and spans many orders of magnitude, so colour the sign and print in scientific notation. */
  void DriftRow(const char *_label, double _drift) noexcept {
    ImGui::TextUnformatted(_label);
    ImGui::SameLine(190.f);
    const ImVec4 color = _drift > 0. ? ImVec4 { 1.f, 0.55f, 0.35f, 1.f } : ImVec4 { 0.45f, 0.75f, 1.f, 1.f };
    ImGui::TextColored(color, "%s", Format("{:+.3e}", _drift).c_str());
  }
} // namespace

void SimHud::OnStart() {
  Scene &scene = m_object->GetScene();
  m_cloud = orb::FindInScene<PointCloud>(scene);
  m_tracer = orb::FindInScene<Tracer>(scene);
  m_camera = orb::FindInScene<OrbitCamera>(scene);

  if (!m_cloud) ROSE_LOG_ERROR("SimHud: no PointCloud in the scene; the panel will be empty.");
}

void SimHud::FrameUpdate() {
  const double dt = Time::deltaTime;

  ++m_fpsFrames;
  m_fpsAccum += dt;
  if (m_fpsAccum >= fpsRefreshInterval) {
    m_fps = m_fpsFrames / m_fpsAccum;
    m_fpsFrames = 0;
    m_fpsAccum = 0.;
  }

  if (!m_cloud) return;

  const SimStats &stats = m_cloud->GetStats();

  m_historyTimer += dt;
  if (m_historyTimer >= historyInterval) {
    m_historyTimer = 0.;
    m_cloudDrift[m_historyHead] = static_cast<float>(m_cloud->GetEnergyDrift());
    m_tracerDrift[m_historyHead] = m_tracer ? static_cast<float>(m_tracer->GetEnergyDrift()) : 0.f;
    m_historyHead = (m_historyHead + 1) % historyLength;
  }

  ImGui::SetNextWindowSize({ 430.f, 0.f }, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos({ 16.f, 16.f }, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.82f);

  if (!ImGui::Begin("Orbits", nullptr, ImGuiWindowFlags_NoTitleBar)) {
    ImGui::End();
    return;
  }

  ImGui::TextColored({ 1.f, 0.86f, 0.55f, 1.f }, "point cloud in a softened Kepler field");
  ImGui::Separator();

  /* ---------------------------------------------------------------- integrator */
  const char *names[static_cast<int>(Integrator::Count)] {};
  for (int i = 0; i < static_cast<int>(Integrator::Count); ++i)
    names[i] = IntegratorName(static_cast<Integrator>(i));

  int integrator = static_cast<int>(m_cloud->GetIntegrator());
  ImGui::SetNextItemWidth(220.f);
  if (ImGui::Combo("integrator", &integrator, names, static_cast<int>(Integrator::Count)))
    m_cloud->SetIntegrator(static_cast<Integrator>(integrator));
  ImGui::TextColored({ 0.65f, 0.7f, 0.8f, 1.f }, "%s", IntegratorNote(m_cloud->GetIntegrator()));

  /* ------------------------------------------------------------- conservation */
  ImGui::Separator();
  ImGui::TextColored({ 0.7f, 0.9f, 1.f, 1.f }, "conserved quantities");

  LabelledValue("E (seeded)", Format("{:.6g}", stats.energy0));
  LabelledValue("E (now)", Format("{:.6g}", stats.energy));
  DriftRow("dE/|E0|", m_cloud->GetEnergyDrift());
  LabelledValue("|L| (now)", Format("{:.6g}", stats.angularMomentum));
  DriftRow("d|L|/|L0|", m_cloud->GetAngularMomentumDrift());

  ImGui::PlotLines("##cloudDrift", m_cloudDrift, static_cast<int>(historyLength), static_cast<int>(m_historyHead),
                   "cloud energy drift", FLT_MAX, FLT_MAX, { 0.f, 56.f });

  if (m_tracer && m_tracer->IsLive()) {
    ImGui::Separator();
    ImGui::TextColored({ 0.5f, 1.f, 0.92f, 1.f }, "tracer - engine Motion, variable step");
    DriftRow("dE/|E0|", m_tracer->GetEnergyDrift());
    ImGui::PlotLines("##tracerDrift", m_tracerDrift, static_cast<int>(historyLength),
                     static_cast<int>(m_historyHead), "tracer energy drift", FLT_MAX, FLT_MAX, { 0.f, 56.f });
  }

  /* -------------------------------------------------------------- controls */
  ImGui::Separator();
  ImGui::TextColored({ 0.7f, 0.9f, 1.f, 1.f }, "simulation");

  int count = static_cast<int>(m_cloud->GetCount());
  ImGui::SetNextItemWidth(220.f);
  ImGui::SliderInt("particles", &count, 100, 60000);
  /* Reseeding is O(N) and throws the conserved baselines away, so wait until the drag finishes. */
  if (ImGui::IsItemDeactivatedAfterEdit()) m_cloud->SetCount(static_cast<size_t>(count));

  auto step = static_cast<float>(m_cloud->GetStep());
  ImGui::SetNextItemWidth(220.f);
  if (ImGui::SliderFloat("step h (s)", &step, 1e-4f, 5e-2f, "%.5f", ImGuiSliderFlags_Logarithmic))
    m_cloud->SetStep(step);

  auto scale = static_cast<float>(m_cloud->GetTimeScale());
  ImGui::SetNextItemWidth(220.f);
  if (ImGui::SliderFloat("time scale", &scale, 0.f, 8.f, "%.2fx")) m_cloud->SetTimeScale(scale);

  auto jitter = static_cast<float>(m_cloud->GetSpeedJitter());
  ImGui::SetNextItemWidth(220.f);
  ImGui::SliderFloat("speed jitter", &jitter, 0.f, 1.f, "%.2f");
  if (ImGui::IsItemDeactivatedAfterEdit()) m_cloud->SetSpeedJitter(jitter);

  auto spread = static_cast<float>(m_cloud->GetInclinationSpread());
  ImGui::SetNextItemWidth(220.f);
  ImGui::SliderFloat("inclination", &spread, 0.f, 1.f, "%.2f");
  if (ImGui::IsItemDeactivatedAfterEdit()) m_cloud->SetInclinationSpread(spread);

  if (ImGui::Button(m_cloud->IsPaused() ? "resume" : "pause", { 96.f, 0.f })) m_cloud->TogglePaused();
  ImGui::SameLine();
  if (ImGui::Button("reseed", { 96.f, 0.f })) m_cloud->Reseed();
  ImGui::SameLine();
  if (ImGui::Button("new seed", { 96.f, 0.f })) m_cloud->SetSeed(m_cloud->GetSeed() + 1);
  if (m_tracer) {
    ImGui::SameLine();
    if (ImGui::Button("tracer", { 96.f, 0.f })) m_tracer->Reset();
  }

  /* ------------------------------------------------------------------ cost */
  ImGui::Separator();
  ImGui::TextColored({ 0.7f, 0.9f, 1.f, 1.f }, "cost");
  LabelledValue("fps", Format("{:.1f}", m_fps));
  LabelledValue("substeps / frame", Format("{}", stats.substeps));
  LabelledValue("integration", Format("{:.2f} ms", stats.stepMilliseconds));
  LabelledValue("sim time", Format("{:.2f} s over {} steps", stats.time, stats.steps));
  LabelledValue("drawn", Format("{} / {}", stats.drawn, m_cloud->GetCount()));

  if (ImGui::CollapsingHeader("keys")) {
    ImGui::TextUnformatted("arrows      orbit the camera");
    ImGui::TextUnformatted("pgup/pgdn   zoom      home  reset view      O  auto-spin");
    ImGui::TextUnformatted("space       pause     R  reseed             N  new seed");
    ImGui::TextUnformatted("1-4         integrator");
    ImGui::TextUnformatted("[ ]         halve / double the step");
    ImGui::TextUnformatted(", .         slower / faster");
    ImGui::TextUnformatted("esc         quit");
  }

  ImGui::End();
}
