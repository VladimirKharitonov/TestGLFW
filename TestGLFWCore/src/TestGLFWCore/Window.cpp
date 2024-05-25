#include "TestGLFWCore/Window.hpp"
#include "TestGLFWCore/Log.hpp"
#include "TestGLFWCore/Camera.hpp"
#include "TestGLFWCore/Rendering/OpenGL/ShaderProgram.hpp"
#include "TestGLFWCore/Rendering/OpenGL/VertexBuffer.hpp"
#include "TestGLFWCore/Rendering/OpenGL/VertexArray.hpp"
#include "TestGLFWCore/Rendering/OpenGL/IndexBuffer.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>


namespace TestGLFW
{
	static bool s_GLFW_initialize = false;

	GLfloat positions_colors[] = {
		-0.5f, -0.5f, 0.0f,		1.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, 0.0f,		0.0f, 1.0f, 1.0f,
		-0.5f,  0.5f, 0.0f,		1.0f, 0.0f, 1.0f,
		 0.5f,  0.5f, 0.0f,		1.0f, 0.0f, 0.0f
	};

	GLuint indices[] = {
		0, 1, 2, 3, 2, 1
	};

	// ��������� ������
	const char* vertex_shader = R"(
		#version 460
		layout(location = 0) in vec3 vertex_position;
		layout(location = 1) in vec3 vertex_color;
		uniform mat4 model_matrix;
		uniform mat4 view_projection_matrix;
		out vec3 color;
		void main()
		{
			gl_Position = view_projection_matrix * model_matrix * vec4(vertex_position, 1.0);
			color = vertex_color;
		}
	)";

	// ����������� ������
	const char* fragment_shader = R"(
		#version 460
		in vec3 color;
		out vec4 frag_color;
		void main()
		{
			frag_color = vec4(color, 1.0);
		}
	)";

	std::unique_ptr<ShaderProgram> p_shader_program;
	std::unique_ptr<VertexBuffer>  p_positions_colors_vbo;
	std::unique_ptr<IndexBuffer>   p_index_buffer;
	std::unique_ptr<VertexArray>   p_vao;
	float scale[3] = { 1.0f, 1.0f, 1.0f };
	float rotate = 0.0f;
	float translate[3] = { 0.0f, 0.0f, 1.0f };

	float camera_position[3] = { 0.0f, 0.0f, 1.0f };
	float camera_rotation[3] = { 0.0f, 0.0f, 0.0f };
	bool perspective_camera = false;
	Camera camera;

	Window::Window(std::string title, const unsigned int width, const unsigned int height)
		: m_data({ std::move(title), width, height })
	{
		int resultCode = init();

		// ������������� ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplOpenGL3_Init();
		ImGui_ImplGlfw_InitForOpenGL(m_pWindow, true);
	}


	Window::~Window()
	{
		shoutdown();
	}


	int Window::init()
	{
		LOG_INFO("Creating window '{0}' with size {1}x{2}", m_data.title, m_data.width, m_data.height);

		// ������������� GLFW
		if (!s_GLFW_initialize)
		{
			if (!glfwInit())
			{
				LOG_CRITICAL("Failed to initialize GLFW!");
				return -1;
			}
			s_GLFW_initialize = true;
		}

		// �������� ������� ����
		m_pWindow = glfwCreateWindow(m_data.width, m_data.height, m_data.title.c_str(), nullptr, nullptr);
		if (!m_pWindow)
		{
			LOG_CRITICAL("Failed to create window '{0}' with size {1}x{2}", m_data.title, m_data.width, m_data.height);
			glfwTerminate();
			return -2;
		}
		glfwMakeContextCurrent(m_pWindow);

		// ������������� GLAD
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			LOG_CRITICAL("Failed to initialize GLAD!");
			return -3;
		}

		glfwSetWindowUserPointer(m_pWindow, &m_data);

		glfwSetWindowSizeCallback(m_pWindow,
			[](GLFWwindow* pWindow, int width, int height)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(pWindow));
				data.width  = width;
				data.height = height;

				EventWindowResize event(width, height);
				data.eventCallbackFn(event);
			});

		glfwSetCursorPosCallback(m_pWindow,
			[](GLFWwindow* pWindow, double x, double y)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(pWindow));

				EventMouseMoved event(x, y);
				data.eventCallbackFn(event);
			});

		glfwSetWindowCloseCallback(m_pWindow,
			[](GLFWwindow* pWindow)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(pWindow));

				EventWindowClose event;
				data.eventCallbackFn(event);
			});

		glfwSetFramebufferSizeCallback(m_pWindow,
			[](GLFWwindow* pWindow, int width, int height)
			{
				glViewport(0, 0, width, height);
			});


		// ���������� ��������� ���������
		p_shader_program = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);
		if (!p_shader_program->isCompiled())
		{
			return false;
		}

		BufferLayout buffer_layout_2vec3
		{
			ShaderDataType::Float3,
			ShaderDataType::Float3
		};

		p_vao = std::make_unique<VertexArray>();
		p_positions_colors_vbo = std::make_unique<VertexBuffer>(
			positions_colors,
			sizeof(positions_colors),
			buffer_layout_2vec3
		);

		p_index_buffer = std::make_unique<IndexBuffer>(
			indices,
			sizeof(indices) / sizeof(GLuint)
		);

		p_vao->add_vertex_buffer(*p_positions_colors_vbo);
		p_vao->set_index_buffer(*p_index_buffer);

		return 0;
	}


	void Window::shoutdown()
	{
		glfwDestroyWindow(m_pWindow);
		glfwTerminate();
	}


	void Window::on_update()
	{
		glClearColor(m_background_color[0],
			m_background_color[1],
			m_background_color[2],
			m_background_color[3]);
		glClear(GL_COLOR_BUFFER_BIT);


		// ���� demo ImGui
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize.x = static_cast<float>(get_width());
		io.DisplaySize.y = static_cast<float>(get_height());

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//ImGui::ShowDemoWindow();


		// ������ ��� ������ ����� ������� ���� ���� opengl
		ImGui::Begin("Background Color Window");
		ImGui::ColorEdit4("Background Color", m_background_color);
		ImGui::SliderFloat3("scale", scale, 0.0f, 2.0f);
		ImGui::SliderFloat("rotate", &rotate, 0.0f, 360.0f);
		ImGui::SliderFloat3("translate", translate, -0.5f, 0.5f);

		ImGui::SliderFloat3("camera position", camera_position, -10.0f, 10.0f);
		ImGui::SliderFloat3("camera rotation", camera_rotation, 0.0f, 360.0f);
		ImGui::Checkbox("Perspective camera", &perspective_camera);


		// ���������
		p_shader_program->bind();

		glm::mat4 scale_matrix(scale[0], 0,		   0,		 0,
							   0,		 scale[1], 0,		 0,
							   0,		 0,		   scale[2], 0,
							   0,		 0,		   0,		 1
		);

		float rotate_in_rad = glm::radians(rotate);
		glm::mat4 rotate_matrix(cos(rotate_in_rad), sin(rotate_in_rad), 0, 0,
							   -sin(rotate_in_rad), cos(rotate_in_rad), 0, 0,
								0,					0,					1, 0,
								0,					0,					0, 1
		);

		glm::mat4 translate_matrix(1,				0,				0,				0,
								   0,				1,				0,				0,
							   	   0,				0,				1,				0,
								   translate[0],	translate[1],	translate[2],	1
		);

		glm::mat4 model_matrix = translate_matrix * rotate_matrix * scale_matrix;
		p_shader_program->setMatrix4("model_matrix", model_matrix);

		camera.set_position_rotation(glm::vec3(camera_position[0], camera_position[1], camera_position[2]), 
									 glm::vec3(camera_rotation[0], camera_rotation[1], camera_rotation[2])
		);
		camera.set_projection_mode(perspective_camera ? Camera::ProjectionMode::Perspective : Camera::ProjectionMode::Orthographic);
		p_shader_program->setMatrix4("view_projection_matrix", camera.get_projection_matrix() * camera.get_view_matrix());

		p_vao->bind();
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(p_vao->get_indices_count()), GL_UNSIGNED_INT, nullptr);

		ImGui::End();


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


		glfwSwapBuffers(m_pWindow);
		glfwPollEvents();
	}
}