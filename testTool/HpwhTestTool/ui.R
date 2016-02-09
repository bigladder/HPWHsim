
# This is the user-interface definition of a Shiny web application.
# You can find out more about building applications with Shiny here:
#
# http://shiny.rstudio.com
#

library(shiny)

shinyUI(fluidPage(

  # Application title
  titlePanel("Heat Pump Water Heaters, Measured and Simulated"),

  # Sidebar with a slider input for number of bins
  sidebarLayout(
    sidebarPanel(
      actionButton("go", "Plot"),
      # selectInput("model", "HPWH Model", choices = sort(unique(as.character(allLong$model)))),
      selectInput("model", "HPWH Model", choices = modelsToUse),
      selectInput("test", "Lab Test", choices = testsToUse),
      selectInput("vars", "Variables", choices = c("Thermocouples", "Average Tank Temp",
                                                   "Draw", "Input Power", "Output Power"),
                  multiple = TRUE, selected = c("Thermocouples", "Average Tank Temp", "Input Power")),
      sliderInput("testlength",
                  "Hours of Test",
                  min = 0,
                  max = 24,
                  value = c(0, 24),
                  step = .5)
    ),

    # Show a plot of the generated distribution
    mainPanel(
      tabsetPanel(
        tabPanel("Lab Data", plotOutput("testPlot"), width = "100%", height = "600px"),
        tabPanel("Field Data", plotOutput("fieldPlot"), width = "100%", height = "600px")
      )
    )
  )
))
