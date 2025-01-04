#include "application.h"

Application::Application() : Shared::Application("hl_bsp_viewer", { Flag::Scene })
{
	CONSOLE->execute("hud_show_fps 1");
	PLATFORM->setTitle(PRODUCT_NAME);

	mViewport3D = std::make_shared<Scene::Viewport3D>();
	getScene()->getRoot()->attach(mViewport3D);

	mCameraController = std::make_shared<Shared::FirstPersonCameraController>(mViewport3D->getCamera());
	mCameraController->setSensivity(1.0f);
	mCameraController->setSpeed(5.0f);

	//mBspFile.loadFromFile("de_dust2.bsp", true);
	mBspFile.loadFromFile("cs_assault.bsp", true);

	auto& textures = mBspFile.getTextures();
	auto& wads = mBspFile.getWADFiles();
	std::unordered_map<TexId, std::shared_ptr<skygfx::Texture>> textures_map;

	for (int tex_id = 0; tex_id < textures.size(); tex_id++)
	{
		auto& texture = textures[tex_id];

		for (auto& wad : wads)
		{
			for (auto& lump : wad->getLumps())
			{
				auto& bf = wad->getBuffer();

				bf.setPosition(lump.filepos);

				auto miptex = bf.read<miptex_t>();

				if (std::string(miptex.name) != std::string(texture.name))
					continue;

				auto start = bf.getPosition();

				bf.seek(miptex.height * miptex.width);
				bf.seek((miptex.height / 2) * (miptex.width / 2));
				bf.seek((miptex.height / 4) * (miptex.width / 4));
				bf.seek((miptex.height / 8) * (miptex.width / 8));
				bf.seek(2); // unknown 2 bytes

				auto picture = bf.getPosition();

				struct Pixel
				{
					uint8_t r;
					uint8_t g;
					uint8_t b;
					uint8_t a;
				};

				std::vector<Pixel> pixels;
				pixels.resize(miptex.width * miptex.height);

				int j = 0;

				for (auto& pixel : pixels)
				{
					bf.setPosition(start + j);
					bf.setPosition(picture + (bf.read<uint8_t>() * 3));

					j += 1;

					pixel.r = bf.read<uint8_t>();
					pixel.g = bf.read<uint8_t>();
					pixel.b = bf.read<uint8_t>();
					pixel.a = 255;

					if (pixel.r == 0 && pixel.g == 0 && pixel.b == 255)
					{
						pixel.a = 0;
						pixel.r = 255;
						pixel.g = 255;
						pixel.b = 255;
					}
				}

				textures_map[tex_id] = std::make_shared<skygfx::Texture>(miptex.width, miptex.height,
					skygfx::PixelFormat::RGBA8UNorm, pixels.data(), true);
			}
		}
	}

	mBspMapEntity = std::make_shared<HL::BspMapEntity>(mBspFile, textures_map);
	mBspMapEntity->setRotation({ glm::radians(-90.0f), 0.0f, 0.0f });
	mViewport3D->addEntity(mBspMapEntity);

#if defined(PLATFORM_MAC)
	PLATFORM->resize(1600, 1200);
#endif

	mTraceDotEntity = std::make_shared<Scene::SingleMeshEntity>();
	mTraceDotEntity->setTopology(skygfx::Topology::LineList);
	mViewport3D->addEntity(mTraceDotEntity);

	std::vector<skygfx::utils::Mesh::Vertex> vertices;
	for (int i = 0; i < 3; i++)
	{
		const glm::vec4 color = { Graphics::Color::Lime, 1.0f };

		skygfx::utils::Mesh::Vertex v0, v1, v2;

		v0.color = color;

		v1.pos[i] = 10.0f;
		v1.color = color;

		v2.pos[i] = -10.0f;
		v2.color = color;

		vertices.push_back(v0);
		vertices.push_back(v1);
		vertices.push_back(v0);
		vertices.push_back(v2);
	}

	mTraceDotEntity->setVertices(vertices);
}

void Application::onFrame()
{
	static bool show_settings = false;

	ImGui::Begin("Settings", nullptr, ImGui::User::ImGuiWindowFlags_Overlay & ~ImGuiWindowFlags_NoInputs);
	ImGui::SetWindowPos(ImGui::User::BottomLeftCorner());

	auto camera = mViewport3D->getCamera();

	if (show_settings)
	{
		ImGui::SliderAngle("Pitch##1", &camera->pitch, -89.0f, 89.0f);
		ImGui::SliderAngle("Yaw##1", &camera->yaw, -180.0f, 180.0f);
		ImGui::SliderFloat("Fov##1", &camera->fov, 1.0f, 180.0f);
		ImGui::DragFloat3("Position##1", (float*)&camera->position);
		ImGui::DragFloat3("WorldUp##1", (float*)&camera->world_up);
	}

	ImGui::Checkbox("Settings", &show_settings);
	ImGui::End();

	glm::vec3 pos = { camera->position.x, -camera->position.z, camera->position.y };

	auto vectors = skygfx::utils::MakePerspectiveCameraVectors(*camera);

	glm::vec3 dir = { vectors.front.x, -vectors.front.z, vectors.front.y };

	auto trace = mBspFile.traceLine(pos, pos + (dir * 8192.0f));
	mTraceDotEntity->setEnabled(trace.fraction > 0.0f);
	mTraceDotEntity->setPosition({ trace.endpos.x, trace.endpos.z, -trace.endpos.y });
}
