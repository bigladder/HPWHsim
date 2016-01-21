
# This is the server logic for a Shiny web application.
# You can find out more about building applications with Shiny here:
#
# http://shiny.rstudio.com
#

library(shiny)

shinyServer(function(input, output, session) {

  # Update test minutes based on test selected
  observe({
    x <- input$test
    y <- input$model
    testMinutes <- max(allLong$minutes[allLong$model == y & allLong$test == x])
    updateSliderInput(session, "testlength", min = 0, max = ceiling(testMinutes / 60))
  })
  
  p <- eventReactive(input$go, {
    onePlot(input$model, input$test, input$vars, input$testlength[1], input$testlength[2])
  })
  output$testPlot <- renderPlot({
    p()
  })
  
#   output$testPlot <- renderPlot({
#    onePlot(input$model, input$test, input$vars, input$testlength)
#   })

})
