import MTree

if __name__ == '__main__':
      kd = [((66477,5983),"Peru"), 
      ((101106,4139), "Bulgaria"), 
      ((84584,3870), "Bosnia and Herzegovina"), 
      ((250528,3673), "Montenegro"), 
      ((103485,3639), "North Macedonia"),
      ((114600,3586), "Hungary"),
      ((200245,3080), "Czechia"),
      ((212561,3030), "Georgia"),
      ((93390,2966), "Romania"),
      ((215221,2910), "Gibraltar"),
      ((102912,2863), "Brazil"),
      ((175688,2733), "San Marino"),
      ((149455,2678), "Croatia"),
      ((124480,2639), "Slovakia"),
      ((116439,2547), "Argentina"),
      ((113938,2547), "Armenia"),
      ((176235,2525), "Lithuania"),
      ((202419,2512), "Slovenia"),
      ((98156,2489), "Colombia"),
      ((148104,2406), "USA")]
      numDimensions = 2
      t = MTree.MTree(kd, numDimensions)
      t.pTree()

