;;;
;;; Copyright (c) 2006-2008 uim Project http://code.google.com/p/uim/
;;;
;;; All rights reserved.
;;;
;;; Redistribution and use in source and binary forms, with or without
;;; modification, are permitted provided that the following conditions
;;; are met:
;;; 1. Redistributions of source code must retain the above copyright
;;;    notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright
;;;    notice, this list of conditions and the following disclaimer in the
;;;    documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of authors nor the names of its contributors
;;;    may be used to endorse or promote products derived from this software
;;;    without specific prior written permission.
;;;
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
;;; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;;; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;;; ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
;;; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;;; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
;;; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
;;; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
;;; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
;;; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
;;; SUCH DAMAGE.
;;;;

(require "util.scm")
(require-custom "chewing-custom.scm")
(require-custom "chewing-key-custom.scm")

;; widgets and actions

;; widgets
(define chewing-widgets '(widget_chewing_input_mode
			  widget_chewing_shape_mode))

;; default activity for each widgets
(define default-widget_chewing_input_mode 'action_chewing_off)
(define default-widget_chewing_shape_mode 'action_chewing_halfshape)

;; actions of widget_chewing_input_mode
(define chewing-input-mode-actions
  '(action_chewing_off
    action_chewing_on))

;; actions of widget_chewing_shape_mode
(define chewing-shape-mode-actions
  '(action_chewing_halfshape
    action_chewing_fullshape))

;;; implementations

(define chewing-halfshape-mode 0)
(define chewing-fullshape-mode 1)

(define chewing-kbd-layout-alist
  '((chewing-kbd-default	. 0)
    (chewing-kbd-hsu		. 1)
    (chewing-kbd-ibm		. 2)
    (chewing-kbd-gin-yieh	. 3)
    (chewing-kbd-et		. 4)
    (chewing-kbd-et26		. 5)
    (chewing-kbd-dvorak		. 6)
    (chewing-kbd-dvorak-hsu	. 7)
    (chewing-kbd-dachen-cp26	. 8)
    (chewing-kbd-hanyu-pinyin	. 9)))

(define chewing-lib-initialized? #f)

(register-action 'action_chewing_off
		 (lambda (mc)
		   (list
		    'off
		    "-"
		    (N_ "off")
		    (N_ "Direct Input Mode")))
		 (lambda (mc)
		   (not (chewing-context-on mc)))
		 (lambda (mc)
		   (chewing-context-set-on! mc #f)))

(register-action 'action_chewing_on
		 (lambda (mc)
		   (let* ((im (chewing-context-im mc))
			  (name (symbol->string (im-name im))))
		     (list
		      'on
		      "O"
		      (N_ "on")
		      (string-append name (N_ " Mode")))))
		 (lambda (mc)
		   (chewing-context-on mc))
		 (lambda (mc)
		   (chewing-context-set-on! mc #t)))

(register-action 'action_chewing_halfshape
		 (lambda (mc)
		   (list
		    'zh-half-shape
		    "H"
		    (N_ "Half shape")
		    (N_ "Half Shape Mode")))
		 (lambda (mc)
		   (let ((mode (chewing-lib-get-shape-mode
				(chewing-context-mc-id mc))))
		     (if (or (not mode)
			     (= mode 0))
		       #t
		       #f)))
		 (lambda (mc)
		   (chewing-lib-set-shape-mode (chewing-context-mc-id mc)
					       chewing-halfshape-mode)))

(register-action 'action_chewing_fullshape
		 (lambda (mc)
		     (list
		      'zh-full-shape
		      "F"
		      (N_ "Full shape")
		      (N_ " Full Shape Mode")))
		 (lambda (mc)
		   (let ((mode (chewing-lib-get-shape-mode
				(chewing-context-mc-id mc))))
		     (if (and
			   mode
			   (= mode 1))
		       #t
		       #f)))
		 (lambda (mc)
		   (chewing-lib-set-shape-mode (chewing-context-mc-id mc)
					       chewing-fullshape-mode)))

;; Update widget definitions based on action configurations. The
;; procedure is needed for on-the-fly reconfiguration involving the
;; custom API
(define chewing-configure-widgets
  (lambda ()
    (register-widget 'widget_chewing_input_mode
		     (activity-indicator-new chewing-input-mode-actions)
		     (actions-new chewing-input-mode-actions))
    (register-widget 'widget_chewing_shape_mode
		     (activity-indicator-new chewing-shape-mode-actions)
		     (actions-new chewing-shape-mode-actions))))

(define chewing-context-rec-spec
  (append
   context-rec-spec
   '((mc-id		#f)
     (on		#f)
     (showing-candidate #f)
     (commit-raw	#f))))

(define chewing-context-alist '())

(define-record 'chewing-context chewing-context-rec-spec)
(define chewing-context-new-internal chewing-context-new)

(define chewing-context-new
  (lambda (id im name)
    (let ((mc (chewing-context-new-internal id im)))
      (if (symbol-bound? 'chewing-lib-init)
	  (set! chewing-lib-initialized? (chewing-lib-init)))
      (if chewing-lib-initialized?
	(begin
	  (chewing-context-set-mc-id! mc (chewing-lib-alloc-context))
	  (set! chewing-context-alist
		(alist-replace
		 (list (chewing-context-mc-id mc) mc) chewing-context-alist))))
      (chewing-context-set-widgets! mc chewing-widgets)
      mc)))

(define chewing-find-mc
  (lambda (mc-id)
    (car (cdr (assoc mc-id chewing-context-alist)))))

(define chewing-update-preedit
  (lambda (mc)
    (if (chewing-context-commit-raw mc)
	(chewing-context-set-commit-raw! mc #f)
	(im-update-preedit mc))))

(define chewing-proc-direct-state
  (lambda (mc key key-state)
   (if (chewing-on-key? key key-state)
       (chewing-context-set-on! mc #t)
       (chewing-commit-raw mc))))

(define chewing-activate-candidate-selector
  (lambda (mc-id nr nr-per-page)
    (let ((mc (chewing-find-mc mc-id)))
      (im-activate-candidate-selector mc nr nr-per-page))))

(define chewing-deactivate-candidate-selector
  (lambda (mc-id)
    (let ((mc (chewing-find-mc mc-id)))
      (im-deactivate-candidate-selector mc))))

(define chewing-shift-page-candidate
  (lambda (mc-id dir)
    (let ((mc (chewing-find-mc mc-id)))
      (im-shift-page-candidate mc dir))))

(define chewing-pushback-preedit
  (lambda (mc-id attr str)
    (let ((mc (chewing-find-mc mc-id)))
      (im-pushback-preedit mc attr str))))

(define chewing-clear-preedit
  (lambda (mc-id)
    (let ((mc (chewing-find-mc mc-id)))
      (im-clear-preedit mc))))

(define chewing-commit
  (lambda (mc-id str)
    (let ((mc (chewing-find-mc mc-id)))
      (im-clear-preedit mc)
      (im-commit mc str))))

(define chewing-get-kbd-layout
  (lambda ()
    (cdr (assq chewing-kbd-layout chewing-kbd-layout-alist))))

(define chewing-commit-raw
  (lambda (mc)
    (im-commit-raw mc)
    (chewing-context-set-commit-raw! mc #t)))

(define chewing-press-key
  (lambda (mc key key-state pressed)
    (let ((mid (chewing-context-mc-id mc))
	  (ukey (if (symbol? key)
    		    (chewing-lib-keysym-to-ukey key)
		    key)))
      (chewing-lib-press-key mid ukey key-state pressed))))

(define chewing-init-handler
  (lambda (id im arg)
    (chewing-context-new id im arg)))

(define chewing-release-handler
  (lambda (mc)
    (let ((mc-id (chewing-context-mc-id mc)))
      (if (number? mc-id)
	  (begin
	    (chewing-lib-free-context mc-id)
	    (set! chewing-context-alist
		  (alist-delete mc-id chewing-context-alist =)))))))

(define chewing-press-key-handler
  (lambda (mc key key-state)
    (if (chewing-context-on mc)
	(if (chewing-press-key mc key key-state #t)
	    #f
	    (if (chewing-off-key? key key-state)
		(let ((mid (chewing-context-mc-id mc)))
		  (chewing-lib-flush mid)
		  (chewing-reset-handler mc)
		  (im-clear-preedit mc)
		  (im-deactivate-candidate-selector mc)
		  (chewing-context-set-on! mc #f))
		(im-commit-raw mc)))
	(chewing-proc-direct-state mc key key-state))
    (chewing-update-preedit mc)))

(define chewing-release-key-handler
  (lambda (mc key key-state)
    (if (or
	 (ichar-control? key)
	 (not (chewing-context-on mc)))
	(chewing-commit-raw mc))))

(define chewing-reset-handler
  (lambda (mc)
    (let ((mid (chewing-context-mc-id mc)))
      (chewing-lib-reset-context mid))))

(define chewing-focus-in-handler
  (lambda (mc)
    (let ((mid (chewing-context-mc-id mc)))
      (if (chewing-context-on mc)
	  (begin
	    (chewing-lib-focus-in-context mid)
	    (im-update-preedit mc))))))

(define chewing-focus-out-handler
  (lambda (mc)
    (let ((mid (chewing-context-mc-id mc)))
      (if (chewing-context-on mc)
	  (begin
	    (chewing-lib-focus-out-context mid)
	    (im-update-preedit mc))))))

(define chewing-heading-label-char-list
  (lambda (style)
    (cond
      ((eq? style 'chewing-cand-selection-asdfghjkl)
       '("A" "S" "D" "F" "G" "H" "J" "K" "L" ";"))
      (else
       '("1" "2" "3" "4" "5" "6" "7" "8" "9" "0")))))

(define chewing-get-candidate-handler
  (lambda (mc idx accel-enum-hint)
    (let* ((mid (chewing-context-mc-id mc))
	   (cand (chewing-lib-get-nth-candidate mid idx))
	   (page-idx (remainder idx
	    			(chewing-lib-get-nr-candidates-per-page mid)))
	   (char-list (chewing-heading-label-char-list
	   	       chewing-candidate-selection-style))
	   (label (if (< page-idx (length char-list))
		      (nth page-idx char-list)
		      "")))
      (list cand label ""))))

(define chewing-set-candidate-index-handler
  (lambda (mc idx)
    #f))

(register-im
 'chewing
 "zh_TW:zh_HK:zh_SG"
 "UTF-8"
 chewing-im-name-label
 chewing-im-short-desc
 #f
 chewing-init-handler
 chewing-release-handler
 context-mode-handler
 chewing-press-key-handler
 chewing-release-key-handler
 chewing-reset-handler
 chewing-get-candidate-handler
 chewing-set-candidate-index-handler
 context-prop-activate-handler
 #f
 chewing-focus-in-handler
 chewing-focus-out-handler
 #f
 #f
 )

(chewing-configure-widgets)
