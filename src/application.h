#pragma once

#include <HL/bspfile.h>
#include <HL/bsp_draw.h>
#include <shared/all.h>

class Application : public Shared::Application,
	public Common::FrameSystem::Frameable
{
public:
	using TexId = int;

	struct Face
	{
		int start;
		int count;
		TexId tex_id;
	};

public:
	Application();

private:
	void onFrame() override;

private:
	BSPFile mBspFile;
	std::shared_ptr<Scene::Viewport3D> mViewport3D;
	std::shared_ptr<HL::BspMapEntity> mBspMapEntity;
	std::shared_ptr<Scene::SingleMeshEntity> mTraceDotEntity;
	std::shared_ptr<Shared::FirstPersonCameraController> mCameraController;
};
