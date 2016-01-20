
# This is the server logic for a Shiny web application.
# You can find out more about building applications with Shiny here:
#
# http://shiny.rstudio.com
#

library(shiny)

shinyServer(function(input, output) {

  output$testPlot <- renderPlot({
   onePlot(input$model, input$test, input$testlength)
  })

})
