#!/usr/bin/clojure
(ns org.vi-server.virtual-touchscreen
  "Virtual touchscreen pad"
  (:import
    (javax.swing 
     Action 
     JButton 
     JFrame 
     JLabel 
     JPanel 
     JTextField
     SwingUtilities
     JCheckBox
     )))

(def field-margins {:left 90, :top 30, :right 10, :bottom 10})
(def unused-area {:left 10, :top 10, :width 70, :height 250})
(def touchers-starting-position {:label-x 20, :label-y 40,   :check-x 40, :check-y 40})
(def touchers-sizes {:label-width 20, :label-height 20, :check-width 20, :check-height 20})
(def next-toucher-increment {:x 0, :y 20})
(def strings [
 {:text "Unused",     :x 15, :y 25}
 {:text "touchers",   :x 15, :y 35}
 {:text "Touch area (approximate)", :x 95, :y 45}
 {:text "Drag touchers into touch area to aim and check checkbox to touch.", :x 90, :y 15}
])
(def num-touchers 10)


(defn in-unused-area? [x y]
 (and 
  (< (:left unused-area) x (+ (:left unused-area) (:width unused-area)))
  (< (:top unused-area) y (+ (:top unused-area) (:height unused-area)))))


(defn create-painted-panel []
 (proxy [JPanel] []
  (paint [g] 
    (let [w (proxy-super getWidth)  h (proxy-super getHeight)]
    (proxy-super paint g)
    (.drawRect g 
     (:left unused-area)
     (:top unused-area)
     (:width unused-area)
     (:height unused-area)
    )
    (->> strings (map #(.drawString g (:text %) (:x %) (:y %))) (doall))

    (.drawRect g 
     (:left field-margins) 
     (:top field-margins) 
     (- w (+ (:left field-margins) (:right field-margins)))
     (- h (+ (:top field-margins) (:bottom field-margins))))
    )
   )
 ))

(defn toucher-moved [i x y]
 (println i x y))

(defn toucher-active [i flag]
 (println i flag)
 )

(defn create-touchscreen-window []
 (let [
  panel (create-painted-panel)
  frame (JFrame.)
  checkboxes (map (fn[i] (let [c (JCheckBox.)]
    (.addChangeListener c (proxy [javax.swing.event.ChangeListener] []
     (stateChanged [e] 
        (toucher-active i (-> c (.getModel) (.isSelected)))
      )  
     ))c))(range num-touchers))
  labels (map (fn[i] (let [l (JLabel. (str i))   c (nth checkboxes i)]
    (.addMouseMotionListener l (proxy [java.awt.event.MouseMotionListener] []
        (mouseMoved [e] )
        (mouseDragged [e]
            (.move l 
             (+ (.getX l) (.getX e)) 
             (+ (.getY l) (.getY e)) 
             )
            (.move c 
             (+ (.getX c) (.getX e)) 
             (+ (.getY c) (.getY e)) 
             )
            (let [x (.getX l)   y (.getY l)   w (.getWidth panel)  h (.getHeight panel)]
             (when (not (in-unused-area? x y))
              (toucher-moved i 
               (int (/ (* (- x (:left field-margins)) 1024) (- w (+ (:right  field-margins) (:left field-margins)))))
               (int (/ (* (- y (:top  field-margins)) 1024) (- h (+ (:bottom field-margins) (:top  field-margins)))))
              )
            ))
        )
    ))
   l)) (range num-touchers))
  ]
  (doto frame 
   (.setSize  800 550)
   (.setContentPane panel)
   (.setTitle "Virtual Touchscreen")
  )
  (doto panel
   (.setLayout nil)
  )
  (doall (map (fn[i]
      (.setBounds (nth labels i) 
        (+ (:label-x touchers-starting-position) (* (:x next-toucher-increment) i))
        (+ (:label-y touchers-starting-position) (* (:y next-toucher-increment) i))
        (:label-width touchers-sizes) 
        (:label-height touchers-sizes) 
      )
      (.setBounds (nth checkboxes i) 
        (+ (:check-x touchers-starting-position) (* (:x next-toucher-increment) i))
        (+ (:check-y touchers-starting-position) (* (:y next-toucher-increment) i))
        (:check-width touchers-sizes) 
        (:check-height touchers-sizes) 
      )
      (.add panel (nth checkboxes i))
      (.add panel (nth labels i))
    ) (range num-touchers)))
  (.setLocationRelativeTo frame nil)
  frame))

(defn -main [] 
 (SwingUtilities/invokeLater (fn [](.show (create-touchscreen-window))))
 )


;;(-main) ;; For starting as executable script
