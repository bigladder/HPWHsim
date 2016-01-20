
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
      selectInput("model", "HPWH Model", choices = unique(as.character(allLong$model))),
      selectInput("test", "Lab Test", choices = unique(as.character(allLong$test))),
      sliderInput("testlength",
                  "% of Test To View",
                  min = 0,
                  max = 1,
                  value = 1)
    ),

    # Show a plot of the generated distribution
    mainPanel(
      plotOutput("testPlot")
    )
  )
))
