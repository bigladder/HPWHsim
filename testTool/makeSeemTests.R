# 

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")

draws <- read.csv(file = "../drawProfiles/drawschedule.csv", skip = 1)

# Each column is another draw profile. Write them all out
lapply(seq_along(draws)[-1], function(i) {
  drawName <- names(draws)[i]
  dir2 <- paste("tests", drawName, sep = "/")
  dir.create(dir2)
  test_minutes <- sum(!is.na(draws[, i]))
  flowDset <- draws[ , c("minutes", drawName)]
  flowDset <- flowDset[!is.na(flowDset[, 2]), ]
  flowDset <- flowDset[flowDset[, 2] > 0, ]

  # Write out the draw schedule
  drawScheduleFile <- paste(dir2, "drawschedule.csv", sep = "/")
  writeLines(text = "default 0", con = drawScheduleFile)
  write.table(flowDset, file = drawScheduleFile, row.names = FALSE, 
              append = TRUE, sep = ",", quote = FALSE)
  
  # Write out the ambient schedule. Do I want to add
  ambientScheduleFile <- paste(dir2, "ambientTschedule.csv", sep = "/")
  writeLines(text = "default 70", con = ambientScheduleFile)
  taDset <- data.frame("minute" = 1:test_minutes)
  taDset$Tintake <- sin(taDset$minute * 2 * pi / 1440) * 20 + 55
  write.table(taDset[, c("minute", "Tintake")], 
              file = ambientScheduleFile, row.names = FALSE, 
              append = TRUE, sep = ",", quote = FALSE)
  
  
  # Write out the evaporator schedule
  evaporatorScheduleFile <- paste(dir2, "evaporatorTschedule.csv", sep = "/")
  writeLines(text = "default 70", con = evaporatorScheduleFile)
  write.table(taDset[, c("minute", "Tintake")], 
              file = evaporatorScheduleFile, row.names = FALSE, 
              append = TRUE, sep = ",", quote = FALSE)
  
  # Write out the inlet schedule
  inletScheduleFile <- paste(dir2, "inletTschedule.csv", sep = "/")
  writeLines(text = "default 55", con = inletScheduleFile)
  
  # Write out the DR schedule
  DRScheduleFile <- paste(dir2, "DRschedule.csv", sep = "/")
  writeLines(text = "default 1\nminutes,OnOff", con = DRScheduleFile)
  
  # What was the setpoint???
  setpoint <- 125
  testInfoFile <- paste(dir2, "testInfo.txt", sep = "/")
  writeLines(text = paste0("length_of_test ", test_minutes, 
                           "\nsetpoint ", setpoint), 
             con = testInfoFile)
  
  
})

