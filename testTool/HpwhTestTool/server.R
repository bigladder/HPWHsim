
# This is the server logic for a Shiny web application.
# You can find out more about building applications with Shiny here:
#
# http://shiny.rstudio.com
#

library(shiny)

shinyServer(function(input, output, session) {

  # Update test minutes based on test selected
  # Also update test choice based on the model selected
  observe({
    abc <- input$test
    def <- input$model
    testMinutes <- max(allLong$minutes[allLong$model == def & allLong$test == abc])
    maxMinutes <- ceiling(testMinutes / 60)
    updateSliderInput(session, "testlength", min = 0, max = maxMinutes, value = c(0, maxMinutes))
  })
  
  observe({
    x <- input$model
    updateSelectInput(session, "test", choices = sort(unique(as.character(allLong$test[allLong$model == input$model]))))
  })
  
  p <- eventReactive(input$go, {
    onePlot(input$model, input$test, input$vars, input$testlength[1], input$testlength[2])
  })
  output$testPlot <- renderPlot({
    p()
  })
  
  p2 <- eventReactive(input$go, {
    fieldPlot(input$model)
  })
  output$fieldPlot <- renderPlot({
    p2()
  })
  
#   output$testPlot <- renderPlot({
#    onePlot(input$model, input$test, input$vars, input$testlength)
#   })

})
