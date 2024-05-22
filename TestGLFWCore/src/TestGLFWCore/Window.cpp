#include "TestGLFWCore/Window.hpp"
#include "TestGLFWCore/Log.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace TestGLFW
{
	static bool s_GLFW_initialize = false;


	Window::Window(std::string title, const unsigned int width, const unsigned int height)
		: m_title(title), 
		  m_width(width),
		  m_height(height)
	{
		int resultCode = init();
	}


	Window::~Window()
	{
		shoutdown();
	}


	int Window::init()
	{
		LOG_INFO("Creating window '{0}' with size {1}x{2}", m_title, m_width, m_height);

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
		m_pWindow = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
		if (!m_pWindow)
		{
			LOG_CRITICAL("Failed to create window '{0}' with size {1}x{2}", m_title, m_width, m_height);
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

		return 0;
	}


	void Window::shoutdown()
	{
		glfwDestroyWindow(m_pWindow);
		glfwTerminate();
	}


	void Window::on_update()
	{
		glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(m_pWindow);
		glfwPollEvents();
	}
}